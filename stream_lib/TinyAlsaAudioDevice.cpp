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
#define LOG_TAG "TinyAlsaAudioDevice"

#include "TinyAlsaAudioDevice.h"
#include <StreamRouteConfig.h>
#include <AudioUtils.h>
#include <SampleSpec.h>
#include <AudioCommsAssert.hpp>
#include <hardware_legacy/power.h>
#include <cutils/log.h>

using android_audio_legacy::SampleSpec;
using android_audio_legacy::AudioUtils;

const char *const TinyAlsaAudioDevice::POWER_LOCK_TAG = "AudioDevice";

pcm *TinyAlsaAudioDevice::getPcmDevice()
{
    AUDIOCOMMS_ASSERT(_pcmDevice != NULL, "NULL tiny alsa device");

    return _pcmDevice;
}

android::status_t TinyAlsaAudioDevice::open(const char *cardName,
                                            uint32_t deviceId,
                                            const StreamRouteConfig &routeConfig,
                                            bool isOut)
{
    AUDIOCOMMS_ASSERT(_pcmDevice == NULL, "Tiny alsa device already opened");
    AUDIOCOMMS_ASSERT(cardName != NULL, "Null card name");

    acquirePowerLock();

    pcm_config config;
    config.rate = routeConfig.rate;
    config.channels = routeConfig.channels;
    config.format = AudioUtils::convertHalToTinyFormat(routeConfig.format);
    config.period_size = routeConfig.periodSize;
    config.period_count = routeConfig.periodCount;
    config.start_threshold = routeConfig.startThreshold;
    config.stop_threshold = routeConfig.stopThreshold;
    config.silence_threshold = routeConfig.silenceThreshold;
    config.avail_min = routeConfig.availMin;

    ALOGD("%s card (%s,%d) with config (rate(%d), format(%d), channels(%d))",
          __FUNCTION__, cardName, deviceId, config.rate, config.format, config.channels);
    ALOGD("%s\t RingBuffer config: periodSize=%d, nbPeriod=%d startTh=%d, stop Th=%d silence Th=%d",
          __FUNCTION__, config.period_size, config.period_count, config.start_threshold,
          config.stop_threshold, config.silence_threshold);

    //
    // Opens the device in BLOCKING mode (default)
    // No need to check for NULL handle, tiny alsa
    // guarantee to return a pcm structure, even when failing to open
    // it will return a reference on a "bad pcm" structure
    //
    uint32_t flags = (isOut ? PCM_OUT : PCM_IN);
    _pcmDevice = pcm_open(AudioUtils::getCardIndexByName(cardName),
                          deviceId, flags, &config);
    if (_pcmDevice && !pcm_is_ready(_pcmDevice)) {

        ALOGE("%s: Cannot open tinyalsa (%s,%d) device for %s stream (error=%s)", __FUNCTION__,
              cardName,
              deviceId,
              isOut ? "output" : "input",
              pcm_get_error(_pcmDevice));
        goto close_device;
    }
    // Prepare the device (ie allocation of the stream)
    if (pcm_prepare(_pcmDevice) != 0) {

        ALOGE("%s: prepare failed with error %s", __FUNCTION__,
              pcm_get_error(_pcmDevice));
        goto close_device;
    }
    ALOGW_IF((config.period_count * config.period_size) != (pcm_get_buffer_size(_pcmDevice)),
             "%s, refine done by alsa, ALSA RingBuffer = %d (frames), "
             "expected by AudioHAL and AudioFlinger = %d (frames)",
             __FUNCTION__, pcm_get_buffer_size(_pcmDevice),
             config.period_count * config.period_size);

    return android::NO_ERROR;

close_device:

    close();
    return android::NO_MEMORY;
}

android::status_t TinyAlsaAudioDevice::close()
{
    if (_pcmDevice == NULL) {

        return android::DEAD_OBJECT;
    }

    ALOGD("%s", __FUNCTION__);
    pcm_close(_pcmDevice);
    _pcmDevice = NULL;

    releasePowerLock();
    return android::OK;
}

void TinyAlsaAudioDevice::acquirePowerLock()
{
    acquire_wake_lock(PARTIAL_WAKE_LOCK, POWER_LOCK_TAG);
}

void TinyAlsaAudioDevice::releasePowerLock()
{
    release_wake_lock(POWER_LOCK_TAG);
}