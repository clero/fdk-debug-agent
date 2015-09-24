/*
********************************************************************************
*                              INTEL CONFIDENTIAL
*   Copyright(C) 2015 Intel Corporation. All Rights Reserved.
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

#include "cAVS/DynamicSizedFirmwareTypes.hpp"

namespace debug_agent
{

/* This class creates firmware objects that match the topology sample available in the SwAS. */
class CavsTopologySample final
{
public:
    static void createInstanceFirmwareObjects(
        std::vector<cavs::dsp_fw::DSModuleInstanceProps> &moduleInstances,
        std::vector<cavs::dsp_fw::GatewayProps> &gateways,
        std::vector<uint32_t> &pipelineIds,
        std::vector<cavs::dsp_fw::DSPplProps> &pipelines,
        std::vector<cavs::dsp_fw::DSSchedulersInfo> &schedulers);

    static void createFirmwareObjects(
        std::vector<cavs::dsp_fw::ModuleEntry> &modules,
        std::vector<char> &fwConfig,
        std::vector<char> &hwConfig);


    static const size_t moduleCount;
    static const size_t maxPplCount;
    static const size_t gatewaysCount;

private:
    CavsTopologySample();
};

}