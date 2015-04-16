/*
 * Copyright (C) 2013-2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "ParameterChangedObserver.hpp"
#include "Parameter.hpp"
#include <NonCopyable.hpp>
#include <KeyValuePairs.hpp>
#include <IStreamInterface.hpp>
#include <AudioCommsAssert.hpp>
#include <utils/Errors.h>
#include <utils/RWLock.h>
#include <list>
#include <map>
#include <string>
#include <vector>

class CParameterMgrPlatformConnector;
class Criterion;
class CriterionType;
class cnode;

namespace intel_audio
{

class ParameterMgrPlatformConnectorLogger;

class AudioPlatformState
    : public ParameterChangedObserver,
      private audio_comms::utilities::NonCopyable
{
private:
    typedef enum
    {
        Route,
        Audio
    } PfwInstance;

    typedef enum
    {
        ParamCriterion,
        ParamRogue
    } ParameterType;

    typedef std::pair<std::string, std::string> AndroidParamMappingValuePair;
    typedef std::pair<int, const char *> CriterionTypeValuePair;
    typedef std::vector<Parameter *>::iterator ParamIterator;
    typedef std::map<std::string, Criterion *>::iterator CriterionMapIterator;
    typedef std::map<std::string, Criterion *>::const_iterator CriterionMapConstIterator;
    typedef std::map<std::string, CriterionType *>::iterator CriterionTypeMapIterator;
    typedef std::map<std::string, CriterionType *>::const_iterator CriteriaTypeMapConstIterator;

    /**
     * This class defines a unary function to be used when looping on the vector of value pairs
     * of a parameter.
     * It will help applying all the pairs (Android-Parameter value - PFW-Parameter value)
     * for the mapping table.
     */
    class SetAndroidParamMappingPairHelper
    {
    public:
        SetAndroidParamMappingPairHelper(Parameter *param)
            : mParamCriterion(param)
        {}

        void operator()(AndroidParamMappingValuePair pair)
        {
            mParamCriterion->setMappingValuePair(pair.first, pair.second);
        }

        Parameter *mParamCriterion;
    };

    /**
     * This class defines a unary function to be used when looping on the vector of parameters
     * It will help checking if the key received in the AudioParameter structure is associated
     * to a Parameter object and if found, the value will be set to this parameter.
     */
    class SetFromAndroidParameterHelper
    {
    public:
        SetFromAndroidParameterHelper(KeyValuePairs *pairs, int *errorCount)
            : mPairs(pairs),
              mErrorCount(errorCount)
        {}

        void operator()(Parameter *param)
        {

            std::string key(param->getKey());
            std::string value;
            if (mPairs->get(key, value) == android::OK) {
                if (!param->setValue(value)) {
                    *mErrorCount += 1;
                }
                // Do not remove the key as nothing forbid to give the same key for 2
                // criteria / rogue parameters...
            }
        }

        KeyValuePairs *mPairs;
        int *mErrorCount;
    };

    /**
     * This class defines a unary function to be used when looping on the vector of parameters
     * It will help checking if the key received in the AudioParameter structure is associated
     * to a Parameter object and if found, the value will be get from this parameter.
     */
    class GetFromAndroidParameterHelper
    {
    public:
        GetFromAndroidParameterHelper(KeyValuePairs *pairsIn,
                                      KeyValuePairs *pairsOut)
            : mPairsIn(pairsIn), mPairsOut(pairsOut)
        {}

        void operator()(Parameter *param)
        {

            std::string key(param->getKey());
            std::string value;
            if (mPairsIn->get(key, value) == android::OK) {

                std::string literalValue;
                if (param->getValue(literalValue)) {

                    mPairsOut->add(key, literalValue);

                    // The key can be safely removed now. Even if the key appears twice in the
                    // config file (meaning associated to more than one criterion/rogue), the value
                    // of the android parameter will be the same.
                    mPairsIn->remove(key);
                }
            }
        }
        KeyValuePairs *mPairsIn;
        KeyValuePairs *mPairsOut;
    };

    /**
     * This class defines a unary function to be used when looping on the vector of parameters
     * It will help checking if the key received in the AudioParameter structure is associated
     * to a Parameter object and if found, the key will be removed in order to be sure all keys
     * received have been taken into account.
     */
    class ClearKeyAndroidParameterHelper
    {
    public:
        ClearKeyAndroidParameterHelper(KeyValuePairs *pairs)
            : mPairs(pairs)
        {}

        void operator()(Parameter *param)
        {
            std::string key(param->getKey());
            std::string value;
            if (mPairs->get(key, value) == android::OK) {
                mPairs->remove(key);
            }
        }
        KeyValuePairs *mPairs;
    };

    /**
     * This class defines a unary function to be used when looping on the vector of parameters
     * It will help syncing all the parameters.
     */
    class SyncParameterHelper
    {
    public:
        SyncParameterHelper() {}

        void operator()(Parameter *param)
        {
            param->sync();
        }
    };

public:
    AudioPlatformState(IStreamInterface *streamInterface);
    virtual ~AudioPlatformState();

    /**
     * Starts the platform state service.
     *
     * @return OK if success, error code otherwise.
     */
    android::status_t start();

    /**
     * Synchronise all parameters (rogue / criteria on Route and Audio PFW)
     * and apply the configuration on Route PFW.
     */
    void sync();

    /**
     * Apply the configuration of the platform on the route parameter manager.
     * Once all the criteria have been set, the client of the platform state must call
     * this function in order to have the route PFW taking into account these criteria.
     */
    void applyPlatformConfiguration();

    /**
     * Generic setParameter handler.
     * It can for example:
     *      -Set the TTY mode.
     * (Direction of the TTY is a bitfield with Downlink and Uplink fields.)
     *      - Set the HAC mode.
     *      - Set the BT headset NREC. (BT device embeds its acoustic algorithms).
     *      - Set the BT headset negociated Band Type.
     * (Band Type results of the negociation between device and the BT HFP headset.)
     *      - Set the BT Enabled flag.
     *      - Set the context awareness status.
     *      - Set the FM state.
     *      - Set the screen state.
     *
     * @param[in] keyValuePairs semicolon separated list of key=value.
     * @param[in] synchronous if set, application of these parameters shall be synchronous.
     *
     * @return OK if these parameters were applyied correctly, error code otherwise.
     */
    android::status_t setParameters(const std::string &keyValuePairs, bool isSynchronous = false);

    /**
     * Get the global parameters of Audio HAL.
     *
     * @param[out] keys: one or more value pair "name=value", semicolon-separated.
     *
     * @return OK if set is successful, error code otherwise.
     */
    std::string getParameters(const std::string &keys);

    /**
     * Checks if the platform state was correctly started (ie the route parameter manager
     * has been instantiated and started correctly).
     *
     * @return true if platform state is started correctly, false otherwise.
     */
    bool isStarted();

    /**
     * Checks if Platform state has changed i.e. at least one of the criterion of one PFW instance
     * has changed.
     */
    bool hasPlatformStateChanged() const;

    /**
     * Print debug information from target debug files
     */
    void printPlatformFwErrorInfo() const;

private:
    /**
     * Adds a Route criterion to the map of criterion of the platform state.
     * It also adds a pair UniqueId - Criterion name to the state change criterion type in order
     * to be able to track a change on any route criterion into Settings file.
     * It asserts if trying to add twice a criterion with the same name.
     *
     * @param[in] routeCriterion to add.
     */
    void addRouteCriterion(Criterion *routeCriterion);

    virtual void parameterHasChanged(const std::string &name);

    /**
     * Clear all the keys found into AudioParameter and in the configuration file.
     * If unknown keys remain, it prints a warn log.
     *
     * @param[in,out] pairs: {key, value} collection.
     */
    void clearKeys(KeyValuePairs *pairs);

    /**
     * Load the criterion configuration file.
     *
     * @param[in] path Criterion conf file path.
     *
     * @return OK is parsing successful, error code otherwise.
     */
    android::status_t loadAudioHalConfig(const char *path);

    /**
     * Returns a human readable name of the PFW instance associated to a given type.
     *
     * @tparam pfw Instance of parameter framework to which the type is linked.
     *
     * @return name of the PFW Instance
     */
    template <PfwInstance pfw>
    const std::string &getPfwInstanceName() const;

    /**
     * Add a criterion type to AudioPlatformState.
     *
     * @tparam pfw Instance of parameter framework to which the criterion type is linked.
     * @param[in] typeName of the PFW criterion type.
     * @param[in] isInclusive attribute of the criterion type.
     */
    template <PfwInstance pfw>
    void addCriterionType(const std::string &typeName, bool isInclusive);

    /**
     * Add a criterion type value pair to AudioPlatformState.
     *
     * @tparam pfw Instance of parameter framework to which the criterion is linked.
     * @param[in] typeName criterion type name to which this value pair is added to.
     * @param[in] numeric part of the value pair.
     * @param[in] literal part of the value pair.
     */
    template <PfwInstance pfw>
    void addCriterionTypeValuePair(const std::string &typeName, uint32_t numeric,
                                   const std::string &literal);

    /**
     * Add a criterion to AudioPlatformState.
     *
     * @tparam pfw Instance of parameter framework to which the parameter is linked.
     * @param[in] name of the PFW criterion.
     * @param[in] typeName criterion type name to which this criterion is associated to.
     * @param[in] defaultLiteralValue of the PFW criterion.
     */
    template <PfwInstance pfw>
    void addCriterion(const std::string &name,
                      const std::string &typeName,
                      const std::string &defaultLiteralValue);
    /**
     * Parse and load the inclusive criterion type from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadInclusiveCriterionType(cnode *root);

    /**
     * Parse and load the exclusive criterion type from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadExclusiveCriterionType(cnode *root);

    /**
     * Add a parameter and its mapping pairs to the Platform state.
     *
     * @param[in] param object to add.
     * @param[in] valuePairs Mapping table between android-parameter values and PFW parameter values
     */
    void addParameter(Parameter *param,
                      const std::vector<AndroidParamMappingValuePair> &valuePairs);

    /**
     * Add a parameter to AudioPlatformState.
     *
     * @tparam pfw Instance of parameter framework to which the parameter is linked.
     * @tparam type of the parameter (i.e. rogue or criterion).
     * @param[in] typeName parameter type.
     * @param[in] paramKey android-parameter key to which this PFW parameter is associated to.
     * @param[in] name of the PFW parameter.
     * @param[in] defaultValue of the PFW parameter.
     * @param[in] valuePairs Mapping table between android-parameter values and PFW parameter values
     */
    template <PfwInstance pfw, ParameterType type>
    void addParameter(const std::string &typeName, const std::string &paramKey,
                      const std::string &name, const std::string &defaultValue,
                      const std::vector<AndroidParamMappingValuePair> &valuePairs);
    /**
     * Parse and load the rogue parameter type from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadRogueParameterType(cnode *root);

    /**
     * Parse and load the rogue parameters type from configuration file and push them into a list.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadRogueParameterTypeList(cnode *root);

    /**
     * Parse and load the criteria from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadCriteria(cnode *root);

    /**
     * Parse and load a criterion from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadCriterion(cnode *root);

    /**
     * Parse and load the criterion types from configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file
     * @param[in] isInclusive true if inclusive, false is exclusive.
     */
    template <PfwInstance pfw>
    void loadCriterionType(cnode *root, bool isInclusive);

    /**
     * Parse and load the chidren node from a given root node.
     *
     * @param[in] root node of the configuration file
     * @param[out] path of the parameter manager element to retrieve.
     * @param[out] defaultValue of the parameter manager element to retrieve.
     * @param[out] key of the android parameter to retrieve.
     * @param[out] type of the parameter manager element to retrieve.
     * @param[out] valuePairs pair of android value / parameter Manager value.
     */
    void parseChildren(cnode *root, std::string &path, std::string &defaultValue, std::string &key,
                       std::string &type, std::vector<AndroidParamMappingValuePair> &valuePairs);

    /**
     * Load the configuration file.
     *
     * @tparam[in] pfw Target Parameter framework for this element.
     * @param[in] root node of the configuration file.
     */
    template <PfwInstance pfw>
    void loadConfigFor(cnode *root);

    template <PfwInstance pfw>
    void loadConfig(cnode *root);

    IStreamInterface *mStreamInterface; /**< Route Manager Stream Interface pointer. */

    /**
     * Parse and load the mapping table of a criterion from configuration file.
     * A mapping table associates the Android Parameter values to the criterion values.
     *
     * @param[in] values string of the list of param value/criterion values comma separated to parse.
     *
     * @return vector of value pairs.
     */
    std::vector<AndroidParamMappingValuePair> parseMappingTable(const char *values);

    /**
     * Check if the given collection has the element indexed by the name key
     *
     * @tparam T type of element to search.
     * @param[in] name name of the element to find.
     * @param[in] elementsMap maps of elements to search into.
     *
     * @return true if element found within collection, false otherwise.
     */
    template <typename T>
    bool collectionHasElement(const std::string &name,
                              const std::map<std::string, T> &collection) const;

    /**
     * Retrieve an element from a map by its name.
     *
     * @tparam T type of element to search.
     * @param[in] name name of the element to find.
     * @param[in] elementsMap maps of elements to search into.
     *
     * @return valid pointer on element if found, NULL otherwise.
     */
    template <typename T>
    T *getElement(const std::string &name, std::map<std::string, T *> &elementsMap);

    /**
     * Retrieve an element from a map by its name. Const version.
     *
     * @tparam T type of element to search.
     * @param[in] name name of the element to find.
     * @param[in] elementsMap maps of elements to search into.
     *
     * @return valid pointer on element if found, NULL otherwise.
     */
    template <typename T>
    const T *getElement(const std::string &name,
                        const std::map<std::string, T *> &elementsMap) const;

    /**
     * set the value of a component state.
     *
     * @param[in] value new value to set to the component state.
     * @param[in] stateName of the component state.
     */
    void setValue(int value, const std::string &stateName);

    /**
     * get the value of a component state.
     *
     * @param[in] name of the component state.
     *
     * @return value of the component state
     */
    int getValue(const std::string &stateName) const;

    /**
     * Resets the platform state events.
     */
    void clearPlatformStateEvents();

    /**
     * Sets a platform state event.
     *
     * @param[in] eventStateName name of the event that happened.
     */
    void setPlatformStateEvent(const std::string &eventStateName);

    std::map<std::string, CriterionType *> mRouteCriterionTypeMap;
    std::map<std::string, Criterion *> mRouteCriterionMap; /**< Route Criterion Map. */
    /** Audio Criterion Map indexed by the name of criterion and storing the name of the type */
    std::map<std::string, std::string> mAudioCriterionMap;
    std::vector<Parameter *> mParameterVector; /**< Map of parameters. */

    CParameterMgrPlatformConnector *mRoutePfwConnector; /**< Route Parameter Manager connector. */
    ParameterMgrPlatformConnectorLogger *mRoutePfwConnectorLogger; /**< Route PFW logger. */

    /**
     * Defines the name of the Android property describing the name of the PFW configuration file.
     */
    static const char *const mRoutePfwConfFileNamePropName;
    static const char *const mRoutePfwDefaultConfFileName; /**< default PFW conf file name. */
    static const std::string mStateChangedCriterionName;  /**< StateChanged route criterion. */
    static const std::string mAndroidModeCriterionName; /**< Input sources criterion name. */

    /**
     * String containing a list of paths to the hardware debug files on target
     * to debug the audio firmware/driver in case of EIO error. Defined in pfw.
     */
    static const std::string mHwDebugFilesPathList;

    /**
     * Max size of the output debug stream in characters
     */
    static const uint32_t mMaxDebugStreamSize;

    /**
     * provide a compile time error if no specialization is provided for a given type.
     *
     * @tparam T: type of the parameter manager element. Supported one are:
     *                      - Criterion
     *                      - CriterionType.
     */
    template <typename T>
    struct parameterManagerElementSupported;

    /**
     * Boolean to indicate that at least one of the audio PFW criterion has changed and the
     * routing must be reconsidered in order to apply configurations that may depend on these
     * criteria.
     */
    bool mAudioPfwHasChanged;

    /**
     * PFW concurrency protection - to garantee atomic operation only.
     */
    mutable android::RWLock mPfwLock;
};

} // namespace intel_audio
