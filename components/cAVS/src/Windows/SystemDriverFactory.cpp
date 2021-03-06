/*
 * Copyright (c) 2015, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "cAVS/SystemDriverFactory.hpp"
#include "cAVS/Windows/Driver.hpp"
#include "cAVS/Windows/LastError.hpp"
#include "cAVS/Windows/SystemDevice.hpp"
#include "cAVS/Windows/DeviceIdFinder.hpp"
#include "cAVS/Windows/RealTimeWppClientFactory.hpp"
#include "cAVS/Windows/StubbedWppClientFactory.hpp"

namespace debug_agent
{
namespace cavs
{

/** OED driver interface substring */
static const std::string DriverInterfaceSubstr = "intelapp2audiodspiface";

/** OED driver class */
const GUID DriverInterfaceGuid = {
    0xd562b888, 0xcf36, 0x4c54, {0x84, 0x1d, 0x10, 0xff, 0x7b, 0xff, 0x4f, 0x60}};

std::unique_ptr<Driver> cavs::SystemDriverFactory::newDriver() const
{
    /* Finding device id */
    std::string deviceId;
    try {
        deviceId = windows::DeviceIdFinder::findOne(DriverInterfaceGuid, DriverInterfaceSubstr);
    } catch (windows::DeviceIdFinder::Exception &e) {
        throw Exception("Cannot get device identifier: " + std::string(e.what()));
    }

    std::unique_ptr<windows::Device> device;
    try {
        device = std::make_unique<windows::SystemDevice>(deviceId);
    } catch (windows::Device::Exception &e) {
        throw Exception("Cannot create device: " + std::string(e.what()));
    }

    /* Creating the WppClientFactory */
    std::unique_ptr<windows::WppClientFactory> wppClientFactory;
    if (mLogControlOnly) {
        wppClientFactory = std::make_unique<windows::StubbedWppClientFactory>();
    } else {
        wppClientFactory = std::make_unique<windows::RealTimeWppClientFactory>();
    }

    try {
        /* Creating Probe Event handles*/
        windows::ProberBackend::EventHandles eventHandles =
            windows::ProberBackend::SystemEventHandlesFactory::createHandles();

        /* Creating Driver interface */
        return std::make_unique<windows::Driver>(std::move(device), std::move(wppClientFactory),
                                                 eventHandles);
    } catch (windows::EventHandle::Exception &e) {
        throw Exception("Cannot create probe event handle : " + std::string(e.what()));
    }
}
}
}
