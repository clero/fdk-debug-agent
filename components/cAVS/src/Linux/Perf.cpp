/*
********************************************************************************
*                              INTEL CONFIDENTIAL
*   Copyright(C) 2016 Intel Corporation. All Rights Reserved.
*   The source code contained  or  described herein and all documents related to
*   the source code ("Material") are owned by Intel Corporation or its suppliers
*   or licensors.  Title to the  Material remains with  Intel Corporation or its
*   suppliers and licensors. The Material contains trade secrets and proprietary
*   and  confidential  information of  Intel or its suppliers and licensors. The
*   Material  is  protected  by  worldwide  copyright  and trade secret laws and
*   treaty  provisions. No part of the Material may be used, copied, reproduced,
*   modified, published, uploaded, posted, transmitted, distributed or disclosed
*   in any way without Intel's prior express written permission.
*   No license  under any  patent, copyright, trade secret or other intellectual
*   property right is granted to or conferred upon you by disclosure or delivery
*   of the Materials,  either expressly, by implication, inducement, estoppel or
*   otherwise.  Any  license  under  such  intellectual property  rights must be
*   express and approved by Intel in writing.
*
********************************************************************************
*/
#include "cAVS/Linux/Perf.hpp"
#include "cAVS/Linux/DriverTypes.hpp" // for driver::CorePowerCommand
#include "Util/ByteStreamReader.hpp"
#include "Util/ByteStreamWriter.hpp"

namespace debug_agent
{
namespace cavs
{
namespace linux
{
void Perf::setState(State state)
{
    if (state == getState()) {
        // Prevents triggering the driver when we are not changing state. This normally isn't
        // useful but it makes it easier for us, since we need to handle an internal state machine.
        // (see below)
        return;
    }

    // Trig a PM action when enabling or disabling the perf service
    if (state == State::Started or state == State::Disabled) {
        // only preventing core 0 from sleeping is enough.
        driver::CorePowerCommand corePowerCmd(state == State::Disabled, 0);
        try {
            mDevice.commandWrite(driver::corePowerCtrl, corePowerCmd.getBuffer());
        } catch (const Device::Exception &e) {
            throw Exception("Error: could not set core power: " + std::string(e.what()));
        }
    }

    util::MemoryByteStreamWriter writer;
    writer.write(static_cast<uint32_t>(state));
    mModuleHandler.configSet(dsp_fw::baseFirmwareModuleId, dsp_fw::baseFirmwareInstanceId,
                             dsp_fw::ParameterId{dsp_fw::BaseFwParams::PERF_MEASUREMENTS_STATE},
                             writer.getBuffer());
}

Perf::State Perf::getState()
{
    auto fromDriver = mModuleHandler.configGet(
        dsp_fw::baseFirmwareModuleId, dsp_fw::baseFirmwareInstanceId,
        dsp_fw::ParameterId{dsp_fw::BaseFwParams::PERF_MEASUREMENTS_STATE}, sizeof(State));
    util::MemoryByteStreamReader reader(fromDriver);
    uint32_t state;
    reader.read(state);
    return static_cast<State>(state);
}
}
}
}