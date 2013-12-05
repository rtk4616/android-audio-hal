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

#include <hardware/audio_effect.h>
#include <NonCopyable.hpp>

class AudioEffectSessionStub;

class AudioEffectStub : private audio_comms::utilities::NonCopyable
{
public:
    AudioEffectStub(const struct effect_interface_s *itfe, const effect_descriptor_t *descriptor);

    virtual ~AudioEffectStub();

    /**
     * Create the effect.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int create();

    /**
     * Initialize the effect.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int init();

    /**
     * Reset the effect.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int reset();

    /**
     * Enable the effect.
     * Called by the framework before the first call to process.
     */
    virtual void enable();

    /**
     * Disable the effect.
     * Called by the framework after the last call to process.
     */
    virtual void disable();

    /**
     * Set Parameter to the effect.
     *
     * @param[in] param parameter to set.
     * @param[in] value to set.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int setParameter(void *param, void *value);

    /**
     * Get the effect parameter.
     *
     * @param[in] param parameter to get.
     * @param[out] size parameter + value size.
     * @param[out] value parameter value read.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int getParameter(void *param, size_t *size, void *value);

    /**
     * Set the effect rendering device.
     *
     * @param[in] device mask of the devices.
     *
     * @return 0 if success, error code otherwise.
     */
    virtual int setDevice(uint32_t device);

    /**
     * Get the effect descriptor.
     *
     * @return effect descriptor structure.
     */
    const effect_descriptor_t *getDescriptor() const { return _descriptor; }

    /**
     * Get the effect UUID.
     *
     * @return Unique effect ID.
     */
    const effect_uuid_t *getUuid() const;

    /**
     * Set a Session to attach to this effect.
     *
     * @param[in] session to attach.
     */
    void setSession(AudioEffectSessionStub *session);

    /**
     * Get Session attached to this effect.
     *
     * @return session.
     */
    AudioEffectSessionStub *getSession() const { return _session; }

    /**
     * Get effect Handle.
     * Note that by design choice of Android, effect interface address is considered
     * as the handle of the effect.
     *
     * @return address the of the effect interface.
     */
    effect_handle_t getHandle() { return (effect_handle_t)(&_itfe); }

private:
    /**
     * Effect Descriptor structure.
     * The effect descriptor contains necessary information to facilitate the enumeration of the
     * effect.
     */
    const effect_descriptor_t *_descriptor;
    const struct effect_interface_s *_itfe; /**< Effect control interface structure. */
    uint32_t _preProcessorId; /**< type of preprocessor. */
    uint32_t _state; /**< state of the effect. */
    AudioEffectSessionStub *_session; /**< Session on which the effect is on. */
};