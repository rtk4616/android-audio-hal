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
#define LOG_TAG "AudioPortGroup"

#include "AudioPortGroup.hpp"
#include "AudioPort.hpp"
#include <AudioCommsAssert.hpp>
#include <utilities/Log.hpp>

using std::string;
using audio_comms::utilities::Log;

namespace intel_audio
{

AudioPortGroup::AudioPortGroup(const string &name)
    : RoutingElement(name),
      mPortList(0)
{
}

AudioPortGroup::~AudioPortGroup()
{
}

void AudioPortGroup::addPortToGroup(AudioPort *port)
{
    AUDIOCOMMS_ASSERT(port != NULL, "Invalid port requested");

    mPortList.push_back(port);

    // Give the pointer on Group port back to the port
    port->addGroupToPort(this);

    Log::Verbose() << __FUNCTION__ << ": added " << port->getName() << " to port group";
}

void AudioPortGroup::blockMutualExclusivePort(const AudioPort *port)
{
    AUDIOCOMMS_ASSERT(port != NULL, "Invalid port requested");

    Log::Verbose() << __FUNCTION__ << ": of port " << port->getName();

    PortListIterator it;

    // Find the applicable route for this route request
    for (it = mPortList.begin(); it != mPortList.end(); ++it) {

        AudioPort *itPort = *it;
        if (port != itPort) {

            itPort->setBlocked(true);
        }
    }
}

} // namespace intel_audio
