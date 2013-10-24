/*
 * INTEL CONFIDENTIAL
 * Copyright © 2013 Intel
 * Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intel’s prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 */
#pragma once


#include "AudioRoute.h"
#include "RoutingStage.h"
#include "RouteInterface.h"
#include "StreamInterface.h"
#include <Direction.hpp>
#include <Observable.hpp>
#include <EventListener.h>
#include <InterfaceImplementer.h>
#include <InterfaceProviderImpl.h>
#include <NonCopyable.hpp>
#include <list>
#include <map>
#include <utils/threads.h>
#include <vector>

class CEventThread;
class CParameterMgrPlatformConnector;
class CParameterMgrPlatformConnectorLogger;
class Stream;
class AudioRoute;
class AudioPortGroup;
class AudioPort;
class AudioStreamRoute;
class Criterion;
class CriterionType;
class AudioParameterHelper;
struct pcm_config;

class AudioRouteManager : public NInterfaceProvider::IInterfaceImplementer,
                          public IEventListener,
                          public audio_comms::utilities::Observable,
                          public audio_comms::utilities::NonCopyable
{
private:
    typedef std::map<std::string, AudioRoute *>::iterator RouteMapIterator;
    typedef std::map<std::string, AudioRoute *>::const_iterator RouteMapConstIterator;
    typedef std::map<std::string, AudioStreamRoute *>::iterator StreamRouteMapIterator;
    typedef std::map<std::string, AudioStreamRoute *>::const_iterator StreamRouteMapConstIterator;
    typedef std::map<std::string, AudioPort *>::iterator PortMapIterator;
    typedef std::map<std::string, AudioPort *>::const_iterator PortMapConstIterator;
    typedef std::map<std::string, AudioPortGroup *>::iterator PortGroupMapIterator;
    typedef std::map<std::string, AudioPortGroup *>::const_iterator PortGroupMapConstIterator;
    typedef std::list<Stream *>::iterator StreamListIterator;
    typedef std::list<Stream *>::const_iterator StreamListConstIterator;
    typedef std::map<std::string, Criterion *>::iterator CriteriaMapIterator;
    typedef std::map<std::string, Criterion *>::const_iterator CriteriaMapConstIterator;
    typedef std::map<std::string, CriterionType *>::iterator CriteriaTypeMapIterator;
    typedef std::map<std::string, CriterionType *>::const_iterator CriteriaTypeMapConstIterator;
public:
    AudioRouteManager();
    virtual ~AudioRouteManager();

    /// Inherited from IInterfaceImplementer
    // Interface populate
    virtual
    void getImplementedInterfaces(NInterfaceProvider::CInterfaceProviderImpl &interfaceProvider);
private:
    /// Interface members
    class RouteInterfaceImpl : public IRouteInterface
    {
    public:
        RouteInterfaceImpl(AudioRouteManager *audioRouteManager)
            : _routeMgr(audioRouteManager) {}

        virtual void addPort(const std::string &name, uint32_t portId)
        {
            _routeMgr->addPort(name, portId);
        }

        virtual void addPortGroup(const std::string &name,
                                  uint32_t groupId,
                                  const std::string &portMember)
        {
            _routeMgr->addPortGroup(name, groupId, portMember);
        }

        virtual void addAudioRoute(const std::string &name, uint32_t routeId,
                                   const std::string &portSrc, const std::string &portDst,
                                   bool isOut)
        {
            _routeMgr->addRoute<AudioRoute>(name, routeId, portSrc, portDst, isOut,
                                            _routeMgr->_routeMap);
        }

        virtual void addAudioStreamRoute(const std::string &name,
                                         uint32_t routeId,
                                         const std::string &portSrc, const std::string &portDst,
                                         bool isOut)
        {
            _routeMgr->addRoute<AudioStreamRoute>(name, routeId, portSrc, portDst, isOut,
                                                  _routeMgr->_streamRouteMap);
        }

        virtual void updateStreamRouteConfig(const std::string &name,
                                             const StreamRouteConfig &config)
        {
            _routeMgr->updateStreamRouteConfig(name, config);
        }

        virtual void addRouteSupportedEffect(const std::string &name, const std::string &effect)
        {
            _routeMgr->addRouteSupportedEffect(name, effect);
        }

        virtual void setRouteApplicable(const std::string &name, bool isApplicable)
        {
            _routeMgr->setRouteApplicable(name, isApplicable);
        }

        virtual void setForcedRoutingStageRequested(const std::string &name, RoutingStage stage)
        {
            _routeMgr->setForcedRoutingStageRequested(name, stage);
        }

        virtual void setPortBlocked(const std::string &name, bool isBlocked)
        {
            _routeMgr->setPortBlocked(name, isBlocked);
        }

        virtual bool addCriterionType(const std::string &name,
                                      bool isInclusive)
        {
            return _routeMgr->addCriterionType(name, isInclusive);
        }

        virtual void addCriterionTypeValuePair(const std::string &name,
                                               const std::string &literal,
                                               uint32_t value)
        {
            _routeMgr->addCriterionTypeValuePair(name, literal, value);
        }

        virtual void addCriterion(const std::string &name, const std::string &criteriaType)
        {
            _routeMgr->addCriterion(name, criteriaType);
        }

        virtual void setParameter(const std::string &name, uint32_t value)
        {
            _routeMgr->setCriterion(name, value);
        }
    private:
        AudioRouteManager *_routeMgr;
    } _routeInterface;

    class StreamInterfaceImpl : public IStreamInterface
    {
    public:
        StreamInterfaceImpl(AudioRouteManager *audioRouteManager)
            : _routeMgr(audioRouteManager) {}

        virtual android::status_t startService()
        {
            return _routeMgr->startService();
        }

        virtual android::status_t stopService()
        {
            return android::OK;
        }

        virtual void addStream(Stream *stream)
        {
            return _routeMgr->addStream(stream);
        }

        virtual void removeStream(Stream *stream)
        {
            return _routeMgr->removeStream(stream);
        }

        virtual void startStream()
        {
            return _routeMgr->reconsiderRouting(true);
        }

        virtual void stopStream()
        {
            return _routeMgr->reconsiderRouting(true);
        }

        virtual void reconsiderRouting()
        {
            return _routeMgr->reconsiderRouting(false);
        }

        virtual android::status_t setVoiceVolume(float gain)
        {
            return _routeMgr->setVoiceVolume(gain);
        }

        virtual Stream *getVoiceOutputStream()
        {
            return _routeMgr->getVoiceOutputStream();
        }

        virtual uint32_t getLatencyInUs(bool isOut, uint32_t flags = 0) const
        {
            return _routeMgr->getLatencyInUs(isOut, flags);
        }

        virtual uint32_t getPeriodInUs(bool isOut, uint32_t flags = 0) const
        {
            return _routeMgr->getPeriodInUs(isOut, flags);
        }
    private:
        AudioRouteManager *_routeMgr;
    } _streamInterface;
private:
    void setCriterion(const std::string &name, uint32_t value);

    /**
     * Add a new port to route manager.
     *
     * @param[in] name port name.
     * @param[in] portId port Id.
     */
    void addPort(const std::string &name, uint32_t portId);

    /**
     * Add a new port group or / and port belonging to this group to route manager.
     *
     * @param[in] name port name.
     * @param[in] portId port Id.
     * @param[in] portMember port belonging to the port group.
     */
    void addPortGroup(const std::string &name, int32_t groupId, const std::string &portMember);

    /**
     * Add an Audio Route to route manager.
     * Called at audio platform discovery.
     *
     * @tparam T: route type (Audio Route or Audio Stream Route).
     * @param[in] name: route name.
     * @param[in] routeId: route identifier.
     * @param[in] portSrc: source port used by route, may be null if no protection needed.
     * @param[in] portDst: destination port used by route, may be null if no protection needed.
     * @param[in] isOut: route direction (true for output, false for input).
     * @param[in] elementsMap: list in which this route needs to be added.
     */
    template <typename T>
    void addRoute(const std::string &name,
                  uint32_t routeId,
                  const std::string &portSrc,
                  const std::string &portDst,
                  bool isOut,
                  std::map<std::string, T *> &elementsMap);

    /**
     * Update the configuration of a stream route.
     * Configuration is not only the card, device to open, but also the pcm configuration to use.
     *
     * @param[in] name: route name.
     * @param[in] config: Route configuration.
     */
    void updateStreamRouteConfig(const std::string &name, const StreamRouteConfig &config);

    /**
     * Add an HW effect supported by a route.
     * It sets the capability of a route to provide audio effect. The route manager will always
     * prefer using HW effect when supported by the route than using SW effects.
     *
     * @param[in] name: name of the route supporting the HW effect.
     * @param[in] name: name of the Effect supported by the audio route.
     */
    void addRouteSupportedEffect(const std::string &name, const std::string &effect);

    /**
     * Adds a criterion type.
     * Called at audio platform discovery.
     *
     * @param[in] name: name of the criterion type.
     * @param[in] isInclusive: true if criterion is inclusive, false if exclusive.
     *
     * @return true if criterion type added, false if criterion type is already added.
     */
    bool addCriterionType(const std::string &name, bool isInclusive);

    /**
     * Adds a criterion type.
     * Called at audio platform discovery.
     *
     * @param[in] name: name of the criterion type.
     * @param[in] isInclusive: true if criterion is inclusive, false if exclusive.
     *
     * @return true if criterion type added, false if criterion type is already added.
     */
    void addCriterionTypeValuePair(const std::string &name,
                                   const std::string &literal,
                                   uint32_t value);

    /**
     * Add a new criterion.
     * Add a new criterion with a specific type. If already added, it will assert.
     * The criterion type must have been added previously unless, it will assert.
     *
     * @param[in] name criterion name.
     * @param[in] criteriaTypeName criterion type referred by its name.
     */
    virtual void addCriterion(const std::string &name, const std::string &criteriaTypeName);

    /**
     * Add a stream to route manager.
     * The route manager keep tracks of streams opened in order to route / unroute them.
     *
     * @param[in] stream stream to be added (for future routing purpose).
     */
    void addStream(Stream *stream);

    /**
     * Remove a stream from route manager.
     * The route manager keep tracks of streams opened in order to route / unroute them.
     *
     * @param[in] stream to be removed.
     */
    void removeStream(Stream *streamToRemove);

    /**
     * Starts the route manager service.
     */
    android::status_t startService();

    bool isStarted() const;

    /**
     * Apply a mute configuration to fasten the first routing.
     * So all default configurations are applied by the PFW at start-up.
     */
    void initRouting();

    /**
     * Sets the voice volume.
     * Called from AudioSystem/Policy to apply the volume on the voice call stream which is
     * platform dependent.
     *
     * @param[in] gain the volume to set in float format in the expected range [0 .. 1.0].
     *                 Note that any attempt to set a value outside this range will return -ERANGE.
     *
     * @return OK if success, error code otherwise.
     */
    android::status_t setVoiceVolume(float gain);

    /**
     * Get the latency introduced by the route for a given stream flags.
     * If no flag is provided, in output only, it will try to return the latency of PRIMARY.
     *
     * @param isOut direction of stream.
     * @param flags qualifier of a stream (input source for an input, output flags instead).
     *
     * @return  latency in microseconds.
     */
    uint32_t getLatencyInUs(bool isOut, uint32_t flags) const;

    /**
     * Get the period size used by the route for a given stream flags.
     * If no flag is provided, in output only, it will try to return the period of PRIMARY.
     *
     * @param isOut direction of stream.
     * @param flags qualifier of a stream (input source for an input, output flags instead).
     *
     * @return  period in microseconds.
     */
    uint32_t getPeriodInUs(bool isOut, uint32_t flags) const;

    const AudioStreamRoute *findMatchingRoute(bool isOut, uint32_t flags) const;

    /**
     * Sets the applicable attribute of an audio route.
     * Based upon settings file of the Route Manager plugin, this function informs the route
     * manager whether a route is applicable or not.
     *
     * @param[in] name: name of the route applicable.
     * @param[in] isApplicable: true if applicable, false otherwise.
     */
    void setRouteApplicable(const std::string &name, bool isApplicable);

    /**
     * Sets the forced routing stage request attribute of an audio route.
     * Based upon settings file of the Route Manager plugin, this function informs the route
     * manager whether a route needs to go through a routing stage or not.
     *
     * @param[in] name: name of the route that needs reconfiguration.
     * @param[in] stage: routing stage requested.
     */
    void setForcedRoutingStageRequested(const std::string &name, RoutingStage stage);

    /**
     * Sets the blocked attribute of an audio port.
     * Based upon settings file of the Route Manager plugin, this function informs the route
     * manager whether a port is blocked or not.
     *
     * @param[in] name: name of the port to be blocked/unblocked.
     * @param[in] isBlocked: true if blocked, false otherwise.
     */
    void setPortBlocked(const std::string &name, bool isBlocked);

    /**
     * Sets a bit referred by an index within a mask.
     *
     * @param[in] isSet if true, the bit will be set, if false, nop (bit will not be cleared).
     * @param[in] index bit index to set.
     * @param[in|out] mask in which the bit must be set.
     */
    void setBit(bool isSet, uint32_t index, uint32_t &mask);

    /**
     * Checks if the routing conditions changed in a given direction.
     *
     * @tparam isOut direction of the route to consider.
     *
     * @return  true if previously enabled routes is different from currently enabled routes
     *               or if any route needs to be reconfigured.
     */
    template <bool isOut>
    inline bool routingHasChanged();

    /**
     * Must be called from WLocked context
     */
    void reconsiderRouting(bool bIsSynchronous = true);

    /**
     * Returns the voice output stream. Used by Input stream to identify the provider of
     * echo reference in case of SW Echo Cancellation.
     *
     * @return valid stream pointer if found, NULL otherwise.
     */
    Stream *getVoiceOutputStream();

    /**
     * From worker thread context
     * This function requests to evaluate the routing for all the streams
     * after a mode change, a modem event ...
     */
    void doReconsiderRouting();

    /**
     *
     * Returns true if the routing scheme has changed, false otherwise.
     *
     * This function override the applicability evaluation of the MetaPFW
     * since further checks are required:
     *     -Ports used strategy and mutual exclusive port
     *     -Streams route are considered as enabled only if a started stream is applicable on it.
     *
     * Returns true if previous enabled route is different from current enabled route
     *              or if any route needs to be reconfigured.
     *         false otherwise (the list of enabled route did not change, no route
     *              needs to be reconfigured).
     *
     */
    bool checkAndPrepareRouting();

    /**
     * Execute 5-steps routing.
     */
    void executeRouting();

    /**
     * Mute the routes.
     * Mute action will be applied on route pointed by ClosingRoutes criterion.
     */
    void executeMuteRoutingStage();

    /**
     * Computes the PFW route criteria to mute the routes.
     * It sets the ClosingRoutes criteria as the routes that were opened before reconsidering
     * the routing, and either will be closed or need reconfiguration.
     * It sets the OpenedRoutes criteria as the routes that were opened before reconsidering the
     * routing, and will remain enabled and do not need to be reconfigured.
     */
    void setRouteCriteriaForMute();

    /**
     * Unmute the routes
     */
    void executeUnmuteRoutingStage();

    /**
     * Performs the configuration of the routes.
     * Change here the devices, the mode, ... all the criteria required for the routing.
     */
    void executeConfigureRoutingStage();

    /**
     * Computes the PFW route criteria to configure the routes.
     * It sets the OpenedRoutes criteria: Routes that will be opened after reconsidering the routing
     * and ClosingRoutes criteria: reset here, as no more closing action required.
     */
    void setRouteCriteriaForConfigure();

    /**
     * Disable the routes
     */
    void executeDisableRoutingStage();

    /**
     * Computes the PFW route criteria to disable the routes.
     * It appends to the closing route criterion the route that were opened before the routing
     * reconsideration and that will not be used anymore after. It also removes from opened routes
     * criterion these routes.
     *
     */
    void setRouteCriteriaForDisable();

    /**
     * Performs the disabling of the route.
     * It only concerns the action that needs to be done on routes themselves, ie detaching
     * streams, closing alsa devices.
     * Disable Routes that were opened before reconsidering the routing and will be closed after
     * or routes that request to be rerouted.
     *
     * @param[in] bIsPostDisable if set, it indicates that the disable happens after unrouting.
     */
    void doDisableRoutes(bool isPostDisable = false);

    /**
     * Performs the post-disabling of the route.
     * It only concerns the action that needs to be done on routes themselves, ie detaching
     * streams, closing alsa devices. Some platform requires to close stream before unrouting.
     * Behavior is encoded in the route itself.
     *
     */
    inline void doPostDisableRoutes()
    {
        doDisableRoutes(true);
    }

    /**
     * Enable the routes.
     */
    void executeEnableRoutingStage();

    /**
     * Performs the enabling of the routes.
     * It only concerns the action that needs to be done on routes themselves, ie attaching
     * streams, opening alsa devices.
     * Enable Routes that were not enabled and will be enabled after the routing reconsideration
     * or routes that requested to be rerouted.
     *
     * @tparam isOut direction of the routes to disable.
     * @param[in] bIsPreEnable if set, it indicates that the enable happens before routing.
     */
    void doEnableRoutes(bool isPreEnable = false);

    /**
     * Performs the pre-enabling of the routes.
     * It only concerns the action that needs to be done on routes themselves, ie attaching
     * streams, opening alsa devices. Some platform requires to open stream before routing.
     * Behavior is encoded in the route itself.
     */
    inline void doPreEnableRoutes()
    {
        doEnableRoutes(true);
    }

    /**
     * Before routing, prepares the attribute of an audio route.
     *
     * @param[in] route AudioRoute to prepare for routing.
     */
    void prepareRoute(AudioRoute *route);

    /**
     * Find and set a stream for an applicable route.
     * It try to associate a streams that must be started and not already routed, with a stream
     * route according to the applicability mask.
     * This mask depends on the direction of the stream:
     *      -Output stream: output Flags
     *      -Input stream: input source.
     *
     * @param[in] route applicable route to be associated to a stream.
     *
     * @return true if a stream was found and attached to the route, false otherwise.
     */
    bool setStreamForRoute(AudioStreamRoute *route);

    /**
     * Add a routing element referred by its name and id to a map. Routing Elements are ports, port
     * groups, route and stream route. Compile time error generated if called with wrong type.
     *
     * @tparam T type of routing element to add.
     * @param[in] name name of the routing element to add.
     * @param[in] id of the routing element to add.
     * @param[in] elementsMap maps of routing elements to add to.
     *
     * @return true if added, false otherwise (already added).
     */
    template <typename T>
    bool addElement(const std::string &name, uint32_t id, std::map<std::string, T *> &elementsMap);

    /**
     * Get a routing element from a map by its name. Routing Elements are ports, port
     * groups, route and stream route. Compile time error generated if called with wrong type.
     *
     * @tparam T type of routing element to search.
     * @param[in] name name of the routing element to find.
     * @param[in] elementsMap maps of routing elements to search into.
     *
     * @return valid pointer on routing element if found, assert if element not found.
     */
    template <typename T>
    T *getElement(const std::string &name, std::map<std::string, T *> &elementsMap);

    /**
     * Find a routing element from a map by its name. Routing Elements are ports, port
     * groups, route and stream route. Compile time error generated if called with wrong type.
     *
     * @tparam T type of routing element to search.
     * @param[in] name name of the routing element to find.
     * @param[in] elementsMap maps of routing elements to search into.
     *
     * @return valid pointer on element if found, NULL otherwise.
     */
    template <typename T>
    T *findElementByName(const std::string &name, std::map<std::string, T *> elementsMap);

    /**
     * Reset the availability of routing elements belonging to a map. Routing Elements are ports,
     * port, groups, route and stream route. Compile time error generated if called with wrong type.
     *
     * @tparam T type of routing element.
     * @param[in] elementsMap maps of routing elements to search into.
     *
     * @return valid pointer on element if found, NULL otherwise.
     */
    template <typename T>
    void resetAvailability(std::map<std::string, T *> elementsMap);

    /**
     * Returns the route Criterion Type.
     *
     * @return handle of route criterion type.
     */
    inline CriterionType *routeCriterionType()
    {
        return _criterionTypesMap[ROUTE_CRITERION_TYPE];
    }

    /**
     * Reset the routing conditions.
     * It backup the enabled routes, resets the route criteria, resets the needReconfigure flags,
     * resets the availability of the routes and ports.
     */
    void resetRouting();

    /// from IEventListener
    virtual bool onEvent(int fd);
    virtual bool onError(int fd);
    virtual bool onHangup(int fd);
    virtual void onAlarm();
    virtual void onPollError();
    virtual bool onProcess(uint16_t event);

    static const std::pair<int, const char *> ROUTING_STAGE_VALUE_PAIRS[];

    /**
     * Defines the name of the Android property describing the name of the *
     * audio PFW configuration file.
     */
    static const char *const AUDIO_PFW_CONF_FILE_PROP_NAME;
    static const char *const AUDIO_PFW_DEFAULT_CONF_FILE_NAME;
    static const char *const ROUTING_LOCKED_PROP_NAME;
    static const char *const
    CLOSING_ROUTE_CRITERION[audio_comms::utilities::Direction::_nbDirections];
    static const char *const
    OPENED_ROUTE_CRITERION[audio_comms::utilities::Direction::_nbDirections];
    static const char *const ROUTE_CRITERION_TYPE;
    static const char *const ROUTING_STAGE;
    static const char *const VOICE_VOLUME;

    CParameterMgrPlatformConnector *_audioPfwConnector; /**< parameter manager connector */
    CParameterMgrPlatformConnectorLogger *_audioPfwConnectorLogger; /**< PFW logger. */

    /**
     * Map of criteria used to pilot the audio PFW.
     */
    std::map<std::string, Criterion *> _criteriaMap;

    Criterion *_routingStageCriterion;
    Criterion *_selectedClosingRoutes[audio_comms::utilities::Direction::_nbDirections];
    Criterion *_selectedOpenedRoutes[audio_comms::utilities::Direction::_nbDirections];

    AudioParameterHelper *_parameterHelper;

    /**
     * array of list of streams opened.
     */
    std::list<Stream *> _streamsList[audio_comms::utilities::Direction::_nbDirections];

    std::map<std::string, CriterionType *> _criterionTypesMap; /**< criterion types map. */

    std::map<std::string, AudioRoute *> _routeMap; /**< map of audio route to manage. */

    std::map<std::string, AudioStreamRoute *> _streamRouteMap; /**< map of audio stream route. */

    std::map<std::string, AudioPort *> _portMap; /**< map of audio ports whose state may change. */

    /**
     * map of mutuel exclusive port groups.
     */
    std::map<std::string, AudioPortGroup *> _portGroupMap;

    CEventThread *_eventThread; /**< worker thread in which routing is running. */

    bool _isStarted; /**< Started service flag. */

    mutable android::RWLock _routingLock; /**< lock to protect the routing. */

    struct
    {
        uint32_t needReflow;  /**< Bitfield of routes that needs to be mute / unmutes. */
        uint32_t needRepath;  /**< Bitfield of routes that needs to be disabled / enabled. */
        uint32_t enabled;     /**< Bitfield of enabled routes. */
        uint32_t prevEnabled; /**< Bitfield of previously enabled routes. */
    } _routes[audio_comms::utilities::Direction::_nbDirections];

    /**
     * Get the need reflow routes mask.
     *
     * @param[in] isOut direction of the route addressed by the request.
     *
     * @return reflow routes mask in the requested direction.
     */
    inline uint32_t needReflowRoutes(bool isOut) const
    {
        return _routes[isOut].needReflow;
    }

    /**
     * Get the need repath routes mask.
     *
     * @param[in] isOut direction of the route addressed by the request.
     *
     * @return repath routes mask in the requested direction.
     */
    inline uint32_t needRepathRoutes(bool isOut) const
    {
        return _routes[isOut].needRepath;
    }

    /**
     * Get the enabled routes mask.
     *
     * @param[in] isOut direction of the route addressed by the request.
     *
     * @return enabled routes mask in the requested direction.
     */
    inline uint32_t enabledRoutes(bool isOut) const
    {
        return _routes[isOut].enabled;
    }

    /**
     * Get the prevously enabled routes mask.
     *
     * @param[in] isOut direction of the route addressed by the request.
     *
     * @return previously enabled routes mask in the requested direction.
     */
    inline uint32_t prevEnabledRoutes(bool isOut) const
    {
        return _routes[isOut].prevEnabled;
    }

    /**
     * provide a compile time error if no specialization is provided for a given type.
     *
     * @tparam T: type of the routingElement. Routing Element supported are:
     *                      - AudioPort
     *                      - AudioPortGroup
     *                      - AudioRoute
     *                      - AudioStreamRoute.
     */
    template <typename T>
    struct routingElementSupported;
};