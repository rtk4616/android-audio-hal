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
#define LOG_TAG "SampleSpec"

#include "SampleSpec.h"
#include <AudioCommsAssert.hpp>
#include <cutils/log.h>
#include <stdint.h>
#include <errno.h>
#include <limits>

using namespace std;

namespace android_audio_legacy
{

#define SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem)                                    \
    AUDIOCOMMS_ASSERT((sampleSpecItem) >= 0 && (sampleSpecItem) < NbSampleSpecItems, \
                      "Invalid Sample Specifications")


SampleSpec::SampleSpec(uint32_t channel,
                       uint32_t format,
                       uint32_t rate)
{
    init(channel, format, rate);
}

SampleSpec::SampleSpec(uint32_t channel,
                       uint32_t format,
                       uint32_t rate,
                       const vector<ChannelsPolicy> &channelsPolicy)
{
    init(channel, format, rate);
    setChannelsPolicy(channelsPolicy);
}

void SampleSpec::init(uint32_t channel,
                      uint32_t format,
                      uint32_t rate)
{
    _channelMask = 0;
    setSampleSpecItem(ChannelCountSampleSpecItem, channel);
    setSampleSpecItem(FormatSampleSpecItem, format);
    setSampleSpecItem(RateSampleSpecItem, rate);
}

// Generic Accessor
void SampleSpec::setSampleSpecItem(SampleSpecItem sampleSpecItem, uint32_t value)
{
    SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem);

    if (sampleSpecItem == ChannelCountSampleSpecItem) {

        AUDIOCOMMS_ASSERT(value < MAX_CHANNELS, "Max channel number reached");

        _channelsPolicy.clear();
        // Reset all the channels policy to copy by default
        for (uint32_t i = 0; i < value; i++) {

            _channelsPolicy.push_back(Copy);
        }
    }
    _sampleSpec[sampleSpecItem] = value;
}

void SampleSpec::setChannelsPolicy(const vector<ChannelsPolicy> &channelsPolicy)
{
    AUDIOCOMMS_ASSERT(channelsPolicy.size() <= _sampleSpec[ChannelCountSampleSpecItem],
                      "Channel policy vector has more channel than sample spec");
    _channelsPolicy = channelsPolicy;
}

SampleSpec::ChannelsPolicy SampleSpec::getChannelsPolicy(uint32_t channelIndex) const
{
    AUDIOCOMMS_ASSERT(channelIndex < _channelsPolicy.size(),
                      "request of channel policy outside channel numbers");
    return _channelsPolicy[channelIndex];
}

uint32_t SampleSpec::getSampleSpecItem(SampleSpecItem sampleSpecItem) const
{
    SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem);
    return _sampleSpec[sampleSpecItem];
}

size_t SampleSpec::getFrameSize() const
{
    return audio_bytes_per_sample(getFormat()) * getChannelCount();
}

size_t SampleSpec::convertBytesToFrames(size_t bytes) const
{
    AUDIOCOMMS_ASSERT(getFrameSize() != 0, "Null frame size");
    return bytes / getFrameSize();
}

size_t SampleSpec::convertFramesToBytes(size_t frames) const
{
    AUDIOCOMMS_ASSERT(getFrameSize() != 0, "Null frame size");
    AUDIOCOMMS_ASSERT(frames <= numeric_limits<size_t>::max() / getFrameSize(),
                      "conversion exceeds limit");
    return frames * getFrameSize();
}

size_t SampleSpec::convertFramesToUsec(uint32_t frames) const
{
    AUDIOCOMMS_ASSERT(getFrameSize() != 0, "Null frame size");
    AUDIOCOMMS_ASSERT((frames / getSampleRate()) <=
                      (numeric_limits<size_t>::max() / USEC_PER_SEC),
                      "conversion exceeds limit");
    return (USEC_PER_SEC * static_cast<uint64_t>(frames)) / getSampleRate();
}

size_t SampleSpec::convertUsecToframes(uint32_t intervalUsec) const
{
    return static_cast<uint64_t>(intervalUsec) * getSampleRate() / USEC_PER_SEC;
}

bool SampleSpec::isSampleSpecItemEqual(SampleSpecItem sampleSpecItem,
                                       const SampleSpec &ssSrc,
                                       const SampleSpec &ssDst)
{
    if (ssSrc.getSampleSpecItem(sampleSpecItem) != ssDst.getSampleSpecItem(sampleSpecItem)) {

        return false;
    }

    return (sampleSpecItem != ChannelCountSampleSpecItem) ||
           ssSrc.getChannelsPolicy() == ssDst.getChannelsPolicy();
}
}  // namespace android
