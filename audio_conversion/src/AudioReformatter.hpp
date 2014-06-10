/*
 * INTEL CONFIDENTIAL
 * Copyright (c) 2013-2014 Intel
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
 * disclosed in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 */
#pragma once

#include "AudioConverter.hpp"

namespace android_audio_legacy
{

class AudioReformatter : public AudioConverter
{

public:
    AudioReformatter(SampleSpecItem sampleSpecItem);

private:
    /**
     * Configures the context of reformatting operation to do.
     *
     * Sets the reformatting operation to do, based on destination sample spec as well
     * as source sample spec.
     *
     * @param[in] ssSrc Source sample spec to reformat.
     * @param[in] ssDst Targeted sample spec.
     *
     * @return status NO_ERROR is configuration is successful, error code otherwise.
     */
    virtual android::status_t configure(const SampleSpec &ssSrc, const SampleSpec &ssDst);

    /**
     * Converts (Reformats) audio samples.
     *
     * Reformatting is made from signed 16-bits depth format to signed 24-bits depth format.
     *
     * @param[in]  src Source buffer containing audio samples to reformat.
     * @param[out] dst Destination buffer for reformatted audio samples.
     * @param[in]  inFrames number of input frames.
     * @param[out] outFrames output frames processed.
     *
     * @return status NO_ERROR is always returned.
     */
    android::status_t convertS16toS24over32(const void *src,
                                            void *dst,
                                            const uint32_t inFrames,
                                            uint32_t *outFrames);

    /**
     * Converts (Reformats) audio samples.
     *
     * Reformatting is made from signed 24-bits depth format to signed 16-bits depth format.
     *
     * @param[in]  src Source buffer containing audio samples to reformat.
     * @param[out] dst Destination buffer for reformatted audio samples.
     * @param[in]  inFrames number of input frames.
     * @param[out] outFrames output frames processed.
     *
     * @return status NO_ERROR is always returned.
     */
    android::status_t convertS24over32toS16(const void *src,
                                            void *dst,
                                            const uint32_t inFrames,
                                            uint32_t *outFrames);

    /**
     * Used to do 8-bits right shitfs during reformatting operation.
     */
    static const uint32_t mReformatterShiftRight8;

    /**
     * Used to do 16-bits left shitfs during reformatting operation.
     */
    static const uint32_t mReformatterShiftLeft16;
};
}  // namespace android