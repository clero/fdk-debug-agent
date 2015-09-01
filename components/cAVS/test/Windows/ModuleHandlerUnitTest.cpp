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

#include "TestCommon/TestHelpers.hpp"
#include "cAVS/Windows/MockedDevice.hpp"
#include "cAVS/Windows/MockedDeviceCommands.hpp"
#include "cAVS/Windows/ModuleHandler.hpp"
#include <catch.hpp>
#include <memory>
#include <iostream>

/**
 * NOTE: test vector buffers are filled with firmware and driver types, maybe a better way
 * would consist in using a bitstream in order to construct ioctl buffers from scratch.
 * To be discussed...
 */

using namespace debug_agent::cavs;
using namespace debug_agent::cavs::windows;

/** Compares type memory content */
template <typename T>
bool memoryEquals(const T &v1, const T &v2)
{
    return memcmp(&v1, &v2, sizeof(T)) == 0;
}

/** Set the memory of one type with arbitrary content */
template <typename T>
void setArbitraryContent(T &value)
{
    uint8_t *buf = reinterpret_cast<uint8_t*>(&value);
    for (std::size_t i = 0; i < sizeof(T); i++) {
        buf[i] = static_cast<uint8_t>(i);
    }
}

/** Produce a module entry vector of the supplied size.
 * Each entry is filled with an arbitrary content. */
std::vector<ModuleEntry> produceModuleEntries(std::size_t expectedModuleCount)
{
    ModuleEntry moduleEntry;
    setArbitraryContent(moduleEntry);

    std::vector<ModuleEntry> entries;
    for (std::size_t i = 0; i < expectedModuleCount; ++i) {
        entries.push_back(moduleEntry);
    }

    return entries;
}

bool isSameGateway(const dsp_fw::GatewayProps &a, const dsp_fw::GatewayProps &b)
{
    return a.attribs == b.attribs && a.id == b.id;
}

/** Perform a module entry ioctl and check the result using the supplied expected module count */
void checkModuleEntryIoctl(windows::ModuleHandler& moduleHandler, std::size_t expectedModuleCount)
{
    /*Successful get module info command */
    std::vector<ModuleEntry> entries;
    CHECK_NOTHROW(
        moduleHandler.getModulesEntries(static_cast<uint32_t>(expectedModuleCount), entries));

    /* Checking result */
    ModuleEntry expectedModuleEntry;
    setArbitraryContent(expectedModuleEntry);
    CHECK(entries.size() == expectedModuleCount);
    for (auto &candidateModuleEntry : entries) {
        CHECK(memoryEquals(candidateModuleEntry, expectedModuleEntry));
    }
}

TEST_CASE("Module handling: getting module entries")
{
    const static uint32_t moduleCount = 2;

    MockedDevice device;

    /* Setting the test vector
     * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error during getting module entries */
    commands.addGetModuleEntriesCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleCount,
        std::vector<ModuleEntry>()); /* unused parameter */

    /* Simulating a driver error during getting module entries */
    commands.addGetModuleEntriesCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleCount,
        std::vector<ModuleEntry>()); /* unused parameter */

    /* Simulating a firmware error during getting module entries */
    commands.addGetModuleEntriesCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        moduleCount,
        std::vector<ModuleEntry>()); /* unused parameter */

    /* Successful get module info command with 2 modules */
    commands.addGetModuleEntriesCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleCount,
        produceModuleEntries(moduleCount));

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error during getting module entries */
    std::vector<ModuleEntry> entries;
    CHECK_THROWS_MSG(moduleHandler.getModulesEntries(moduleCount, entries),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(entries.empty());

    /* Simulating a driver error during getting module entries */
    CHECK_THROWS_MSG(moduleHandler.getModulesEntries(moduleCount, entries),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(entries.empty());

    /* Simulating a firmware error during getting module entries */
    CHECK_THROWS_MSG(moduleHandler.getModulesEntries(moduleCount, entries),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));

    CHECK(entries.empty());

    /*Successful get module info command with 2 modules*/
    checkModuleEntryIoctl(moduleHandler, 2);
}

TEST_CASE("Module handling: getting FW configs")
{
    static const size_t fwVersionValueOffsetInTlv = 8;

    static const std::vector<char> fwConfigTlvList{
        /* Tag for FW_VERSION: 0x00000000 */
        0x00, 0x00, 0x00, 0x00,
        /* Length = 8 bytes */
        0x08, 0x00, 0x00, 0x00,
        /* Value: dsp_fw::FwVersion */
        /* major and minor */
        0x01, 0x02, 0x03, 0x04,
        /* hot fix and build */
        0x05, 0x06, 0x07, 0x08
    };

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error during getting fw config */
    commands.addGetFwConfigCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwConfigTlvList); /* unused parameter */

    /* Simulating a driver error during getting fw config */
    commands.addGetFwConfigCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwConfigTlvList); /* unused parameter */

    /* Simulating a firmware error during getting fw config */
    commands.addGetFwConfigCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        fwConfigTlvList); /* unused parameter */

    /* Successful get fw config command */
    commands.addGetFwConfigCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwConfigTlvList);


    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error during getting fw config */
    FwConfig fwConfig;
    CHECK_THROWS_MSG(moduleHandler.getFwConfig(fwConfig),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(fwConfig.isFwVersionValid == false);

    /* Simulating a driver error during getting fw config */
    CHECK_THROWS_MSG(moduleHandler.getFwConfig(fwConfig),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(fwConfig.isFwVersionValid == false);

    /* Simulating a firmware error during getting fw config */
    CHECK_THROWS_MSG(moduleHandler.getFwConfig(fwConfig),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(fwConfig.isFwVersionValid == false);

    /* Successful get fw config command */
    CHECK_NOTHROW(moduleHandler.getFwConfig(fwConfig));
    CHECK(fwConfig.isFwVersionValid == true);
    const dsp_fw::FwVersion *injectedVersion =
        reinterpret_cast<const dsp_fw::FwVersion *>
        (fwConfigTlvList.data() + fwVersionValueOffsetInTlv);
    // No operator== in FW type: compare each field individually:
    CHECK(fwConfig.fwVersion.major == injectedVersion->major);
    CHECK(fwConfig.fwVersion.minor == injectedVersion->minor);
    CHECK(fwConfig.fwVersion.hotfix == injectedVersion->hotfix);
    CHECK(fwConfig.fwVersion.build == injectedVersion->build);
}

TEST_CASE("Module handling: getting pipeline list")
{
    static const uint32_t fwMaxPplCount = 10;
    static const std::vector<uint32_t> fwPipelineIdList = { 1, 2, 3 };

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error during getting pipeline list */
    commands.addGetPipelineListCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwMaxPplCount,
        std::vector<uint32_t>()); /* unused parameter */

    /* Simulating a driver error during getting pipeline list  */
    commands.addGetPipelineListCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwMaxPplCount,
        std::vector<uint32_t>()); /* unused parameter */

    /* Simulating a firmware error during getting pipeline list  */
    commands.addGetPipelineListCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        fwMaxPplCount,
        std::vector<uint32_t>()); /* unused parameter */

    /* Successful get pipeline list command */
    commands.addGetPipelineListCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwMaxPplCount,
        fwPipelineIdList);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error during getting pipeline list */
    std::vector<uint32_t> pipelineIds;
    static const uint32_t maxPipeline = 10;
    CHECK_THROWS_MSG(moduleHandler.getPipelineIdList(maxPipeline, pipelineIds),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(pipelineIds.empty());

    /* Simulating a driver error during getting pipeline list */
    CHECK_THROWS_MSG(moduleHandler.getPipelineIdList(maxPipeline, pipelineIds),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(pipelineIds.empty());

    /* Simulating a firmware error during getting pipeline list */
    CHECK_THROWS_MSG(moduleHandler.getPipelineIdList(maxPipeline, pipelineIds),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(pipelineIds.empty());

    /*Successful get pipeline list command */
    CHECK_NOTHROW(moduleHandler.getPipelineIdList(maxPipeline, pipelineIds));
    CHECK(fwPipelineIdList == pipelineIds);
}

TEST_CASE("Module handling: getting pipeline props")
{
    static const uint32_t pipelineId = 1;
    static const DSPplProps fwProps = { 1, 2, 3, 4, 5, 6, { 1, 2, 3 }, { 4, 5 }, {} };

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addGetPipelinePropsCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        pipelineId,
        DSPplProps()); /* unused parameter */

    /* Simulating a driver error */
    commands.addGetPipelinePropsCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        pipelineId,
        DSPplProps()); /* unused parameter */

    /* Simulating a firmware error */
    commands.addGetPipelinePropsCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        pipelineId,
        DSPplProps()); /* unused parameter */

    /* Successful command */
    commands.addGetPipelinePropsCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        pipelineId,
        fwProps);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error */

    static const DSPplProps emptyProps = { 0, 0, 0, 0, 0, 0, {}, {}, {} };
    DSPplProps props = emptyProps;

    CHECK_THROWS_MSG(moduleHandler.getPipelineProps(pipelineId, props),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(emptyProps == props);

    /* Simulating a driver error */
    CHECK_THROWS_MSG(moduleHandler.getPipelineProps(pipelineId, props),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(emptyProps == props);

    /* Simulating a firmware error */
    CHECK_THROWS_MSG(moduleHandler.getPipelineProps(pipelineId, props),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(emptyProps == props);

    /*Successful command */
    CHECK_NOTHROW(moduleHandler.getPipelineProps(pipelineId, props));
    CHECK(props == fwProps);
}

TEST_CASE("Module handling: getting schedulers info")
{
    static const uint32_t coreId = 1;

    static const DSTaskProps task1 = { 3, { 1, 2 } };
    static const DSTaskProps task2 = { 4, { 8 } };
    static const DSTaskProps task3 = { 6, {} };

    static const DSSchedulerProps props1 = { 1, 2, { task1, task2 } };
    static const DSSchedulerProps props2 = { 4, 2, { task3 } };

    static const DSSchedulersInfo fwSchedulersInfo = { { props1, props2 } };

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addGetSchedulersInfoCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        coreId,
        DSSchedulersInfo()); /* unused parameter */

    /* Simulating a driver error */
    commands.addGetSchedulersInfoCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        coreId,
        DSSchedulersInfo()); /* unused parameter */

    /* Simulating a firmware error */
    commands.addGetSchedulersInfoCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        coreId,
        DSSchedulersInfo()); /* unused parameter */

    /* Successful command */
    commands.addGetSchedulersInfoCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        coreId,
        fwSchedulersInfo);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error */

    static const DSSchedulersInfo emptyInfo = {};
    DSSchedulersInfo info = emptyInfo;

    CHECK_THROWS_MSG(moduleHandler.getSchedulersInfo(coreId, info),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(emptyInfo == info);

    /* Simulating a driver error */
    CHECK_THROWS_MSG(moduleHandler.getSchedulersInfo(coreId, info),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(emptyInfo == info);

    /* Simulating a firmware error */
    CHECK_THROWS_MSG(moduleHandler.getSchedulersInfo(coreId, info),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(emptyInfo == info);

    /*Successful command */
    CHECK_NOTHROW(moduleHandler.getSchedulersInfo(coreId, info));
    CHECK(fwSchedulersInfo == info);
}

TEST_CASE("Module handling: getting gateways")
{
    static const uint32_t fwGatewayCount = 10;
    static const std::vector<dsp_fw::GatewayProps> fwGateways = { { 1, 2 }, { 3, 4 } };

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addGetGatewaysCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwGatewayCount,
        std::vector<dsp_fw::GatewayProps>()); /* unused parameter */

    /* Simulating a driver error during */
    commands.addGetGatewaysCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwGatewayCount,
        std::vector<dsp_fw::GatewayProps>()); /* unused parameter */

    /* Simulating a firmware error during */
    commands.addGetGatewaysCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        fwGatewayCount,
        std::vector<dsp_fw::GatewayProps>()); /* unused parameter */

    /* Successful command */
    commands.addGetGatewaysCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        fwGatewayCount,
        fwGateways);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error during getting pipeline list */
    std::vector<dsp_fw::GatewayProps> gateways;
    CHECK_THROWS_MSG(moduleHandler.getGatewaysInfo(fwGatewayCount, gateways),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(gateways.empty());

    /* Simulating a driver error during getting pipeline list */
    CHECK_THROWS_MSG(moduleHandler.getGatewaysInfo(fwGatewayCount, gateways),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(gateways.empty());

    /* Simulating a firmware error during getting pipeline list */
    CHECK_THROWS_MSG(moduleHandler.getGatewaysInfo(fwGatewayCount, gateways),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(gateways.empty());

    /*Successful get pipeline list command */
    CHECK_NOTHROW(moduleHandler.getGatewaysInfo(fwGatewayCount, gateways));
    CHECK(fwGateways.size() == gateways.size());
    CHECK(std::equal(fwGateways.begin(), fwGateways.end(), gateways.begin(), isSameGateway));
}

TEST_CASE("Module handling: getting module instance properties")
{
    static const dsp_fw::AudioDataFormatIpc audioFormat = {
        static_cast<dsp_fw::SamplingFrequency>(1),
        static_cast<dsp_fw::BitDepth>(2),
        static_cast<dsp_fw::ChannelMap>(3),
        static_cast<dsp_fw::ChannelConfig>(4),
        static_cast<dsp_fw::InterleavingStyle>(5),
        6,
        7,
        static_cast<dsp_fw::SampleType>(8),
        9
    };

    static const DSPinListInfo input_pins = { {
        { static_cast<dsp_fw::StreamType>(1), audioFormat, 3 }
    } };
    static const DSPinListInfo output_pins = { {
        { static_cast<dsp_fw::StreamType>(4), audioFormat, 5 },
        { static_cast<dsp_fw::StreamType>(6), audioFormat, 7 }
    } };

    static const DSModuleInstanceProps fwInstanceProps = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, input_pins, output_pins,
        dsp_fw::ConnectorNodeId(12), dsp_fw::ConnectorNodeId(13)
    };

    static const uint16_t moduleId = 1;
    static const uint16_t instanceId = 2;

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addGetModuleInstancePropsCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId,
        DSModuleInstanceProps()); /* unused parameter */

    /* Simulating a driver error */
    commands.addGetModuleInstancePropsCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId,
        DSModuleInstanceProps()); /* unused parameter */

    /* Simulating a firmware error */
    commands.addGetModuleInstancePropsCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        moduleId, instanceId,
        DSModuleInstanceProps()); /* unused parameter */

    /* Successful command */
    commands.addGetModuleInstancePropsCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId,
        fwInstanceProps);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error */

    static const DSModuleInstanceProps emptyProps =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, DSPinListInfo(), DSPinListInfo(),
        dsp_fw::ConnectorNodeId(0), dsp_fw::ConnectorNodeId(0)
    };
    DSModuleInstanceProps props = emptyProps;

    CHECK_THROWS_MSG(moduleHandler.getModuleInstanceProps(moduleId, instanceId, props),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(emptyProps == props);

    /* Simulating a driver error */
    CHECK_THROWS_MSG(moduleHandler.getModuleInstanceProps(moduleId, instanceId, props),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(emptyProps == props);

    /* Simulating a firmware error */
    CHECK_THROWS_MSG(moduleHandler.getModuleInstanceProps(moduleId, instanceId, props),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(emptyProps == props);

    /*Successful command */
    CHECK_NOTHROW(moduleHandler.getModuleInstanceProps(moduleId, instanceId, props));
    CHECK(fwInstanceProps == props);

}

TEST_CASE("Module handling: getting module parameter")
{
    static const std::vector<uint8_t> fwParameterPayload = { 1, 2, 3 };

    static const uint16_t moduleId = 1;
    static const uint16_t instanceId = 2;
    static const uint32_t parameterId = 2;

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addGetModuleParameterCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        std::vector<uint8_t>()); /* unused parameter */

    /* Simulating a driver error */
    commands.addGetModuleParameterCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        std::vector<uint8_t>()); /* unused parameter */

    /* Simulating a firmware error */
    commands.addGetModuleParameterCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        moduleId, instanceId, parameterId,
        std::vector<uint8_t>()); /* unused parameter */

    /* Successful command */
    commands.addGetModuleParameterCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        fwParameterPayload);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error */
    std::vector<uint8_t> parameterPayload;

    CHECK_THROWS_MSG(moduleHandler.getModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Device returns an exception: OS says that io control has failed.");
    CHECK(parameterPayload.empty());

    /* Simulating a driver error */
    CHECK_THROWS_MSG(moduleHandler.getModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));
    CHECK(parameterPayload.empty());

    /* Simulating a firmware error */
    CHECK_THROWS_MSG(moduleHandler.getModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));
    CHECK(parameterPayload.empty());

    /*Successful command */
    CHECK_NOTHROW(moduleHandler.getModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload));
    CHECK(fwParameterPayload == parameterPayload);
}

TEST_CASE("Module handling: setting module parameter")
{
    static const std::vector<uint8_t> parameterPayload = { 4, 5, 6 };

    static const uint16_t moduleId = 1;
    static const uint16_t instanceId = 2;
    static const uint32_t parameterId = 2;

    MockedDevice device;

    /* Setting the test vector
    * ----------------------- */
    MockedDeviceCommands commands(device);

    /* Simulating an os error */
    commands.addSetModuleParameterCommand(
        false,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        parameterPayload);

    /* Simulating a driver error */
    commands.addSetModuleParameterCommand(
        true,
        STATUS_FLOAT_DIVIDE_BY_ZERO,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        parameterPayload);

    /* Simulating a firmware error */
    commands.addSetModuleParameterCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_FAILURE,
        moduleId, instanceId, parameterId,
        parameterPayload);

    /* Successful command */
    commands.addSetModuleParameterCommand(
        true,
        STATUS_SUCCESS,
        dsp_fw::Message::IxcStatus::ADSP_IPC_SUCCESS,
        moduleId, instanceId, parameterId,
        parameterPayload);

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the module handler, that will use the mocked device*/
    windows::ModuleHandler moduleHandler(device);

    /* Simulating an os error */
    CHECK_THROWS_MSG(moduleHandler.setModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Device returns an exception: OS says that io control has failed.");

    /* Simulating a driver error */
    CHECK_THROWS_MSG(moduleHandler.setModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Driver returns invalid status: " +
        std::to_string(static_cast<uint32_t>(STATUS_FLOAT_DIVIDE_BY_ZERO)));

    /* Simulating a firmware error */
    CHECK_THROWS_MSG(moduleHandler.setModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload),
        "Firmware returns invalid status: " +
        std::to_string(static_cast<uint32_t>(dsp_fw::Message::ADSP_IPC_FAILURE)));

    /*Successful command */
    CHECK_NOTHROW(moduleHandler.setModuleParameter(moduleId, instanceId, parameterId,
        parameterPayload));
}


