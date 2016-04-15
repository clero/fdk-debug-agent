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
#include "cAVS/Windows/Perf.hpp"
#include "cAVS/Windows/IoCtlDescription.hpp"
#include "cAVS/Windows/IoctlHelpers.hpp"

namespace debug_agent
{
namespace cavs
{
namespace windows
{
using ioctl_helpers::ioctl;
using driver::IoCtlType;
using driver::IOCTL_FEATURE;
using GetState =
    IoCtlDescription<IoCtlType::TinyGet, static_cast<IOCTL_FEATURE>(0x270000), 0, uint32_t>;
using SetState =
    IoCtlDescription<IoCtlType::TinySet, static_cast<IOCTL_FEATURE>(0x270000), 0, uint32_t>;

void Perf::setState(Perf::State state)
{
    auto hack = static_cast<SetState::Data>(state);
    ioctl<SetState, Perf::Exception>(mDevice, hack);
}

Perf::State Perf::getState()
{
    GetState::Data state{0xffffffff};
    ioctl<GetState, Perf::Exception>(mDevice, state);
    return static_cast<Perf::State>(state);
}
}
}
}
