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
#pragma once

#include "cAVS/Perf.hpp"
#include "cAVS/ModuleHandler.hpp"
#include <vector>
#include "Util/Exception.hpp"

namespace debug_agent
{
namespace cavs
{
class PerfService
{
public:
    struct CompoundPerfData
    {
        std::vector<Perf::Item> cores, modules;
    };
    using Exception = util::Exception<PerfService, Perf::Exception>;

    PerfService(Perf &perf, ModuleHandler &moduleHandler);

    void setMaxItemCount(uint32_t maxItemCount);

    /** @throws Perf::Exception */
    /** @{ */
    Perf::State getState();
    void setState(Perf::State);

    /** @returns performance measurement data. */
    CompoundPerfData getData();
    /** @} */

private:
    Perf &mPerf;
    ModuleHandler &mModuleHandler;

    uint32_t mMaxItemCount{0};
};
}
}