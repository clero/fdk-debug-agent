/*
 * Copyright (c) 2015-2016, Intel Corporation
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

#include "CavsTopologySample.hpp"
#include "Core/DebugAgent.hpp"
#include "Util/Uuid.hpp"
#include "Util/StringHelper.hpp"
#include "Util/FileHelper.hpp"
#include "TestCommon/HttpClientSimulator.hpp"
#include "TestCommon/TestHelpers.hpp"
#include "cAVS/Windows/DeviceInjectionDriverFactory.hpp"
#include "cAVS/Windows/MockedDevice.hpp"
#include "cAVS/Windows/MockedDeviceCommands.hpp"
#include "cAVS/Windows/StubbedWppClientFactory.hpp"
#include "cAVS/Windows/EventHandle.hpp"
#include "cAVS/Windows/TestEventHandle.hpp"
#include "System/IfdkStreamHeader.hpp"
#include "catch.hpp"
#include <chrono>
#include <thread>
#include <future>
#include <condition_variable>
#include <fstream>

using namespace debug_agent;
using namespace debug_agent::core;
using namespace debug_agent::cavs;
using namespace debug_agent::test_common;
using namespace debug_agent::util;

static const util::Buffer aecControlParameterPayload = {
    0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0xF1, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xF1, 0xFF,
    0xF1, 0xFF, 0xF1, 0xFF, 0x00, 0x00, 0xF1, 0xFF, 0xF1, 0xFF, 0x00, 0x00, 0xF1, 0xFF, 0x00, 0x00,
    0xF4, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
    0x00, 0x80, 0xF1, 0xFF, 0xF1, 0xFF, 0xF1, 0xFF, 0xF1, 0xFF, 0xF1, 0xFF, 0xF1, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xAA};
static const util::Buffer nsControlParameterPayload = {
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xF1,
    0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const std::string dataPath = PROJECT_PATH "data/FunctionalTests/http/";

std::string xmlFileName(const std::string &name)
{
    return dataPath + name + ".xml";
}

std::string htmlFileName(const std::string &name)
{
    return dataPath + name + ".html";
}

const std::string &pfwConfigPath =
    PROJECT_PATH "data/FunctionalTests/pfw/ParameterFrameworkConfigurationDBGA.xml";

/** Helper function to set a module entry */
void setModuleEntry(dsp_fw::ModuleEntry &entry, const std::string &name, const Uuid &uuid)
{
    /* Setting name */
    StringHelper::setStringToFixedSizeArray(entry.name, sizeof(entry.name), name);

    /* Setting GUID*/
    uuid.toOtherUuidType(entry.uuid);
}

/** Handle DebugAgent initial and final ioctl commands */
class DBGACommandScope
{
public:
    DBGACommandScope(windows::MockedDeviceCommands &commands) : mCommands(commands)
    {
        /* Constructing cavs model */
        /* ----------------------- */
        std::vector<dsp_fw::ModuleEntry> modules;
        Buffer fwConfig;
        Buffer hwConfig;

        CavsTopologySample::createFirmwareObjects(modules, fwConfig, hwConfig);

        /* Adding initial commands */
        mCommands.addGetFwConfigCommand(true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                        fwConfig);
        mCommands.addGetHwConfigCommand(true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                        hwConfig);
        mCommands.addGetModuleEntriesCommand(true, STATUS_SUCCESS,
                                             dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                             static_cast<uint32_t>(modules.size()), modules);
    }

    ~DBGACommandScope()
    {
        // Upon Perf service destruction, it is disabled in the firmware
        mCommands.addSetPerfState(true, STATUS_SUCCESS, Perf::State::Disabled);

        // When the probe service is destroyed, it checks if the driver service state is Idle
        mCommands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                          windows::driver::ProbeState::ProbeFeatureIdle);
    }

private:
    windows::MockedDeviceCommands &mCommands;
};

void addInstanceTopologyCommands(windows::MockedDeviceCommands &commands)
{
    std::vector<dsp_fw::ModuleInstanceProps> moduleInstances;
    std::vector<dsp_fw::GatewayProps> gateways;
    std::vector<dsp_fw::PipeLineIdType> pipelineIds;
    std::vector<dsp_fw::PplProps> pipelines;
    std::vector<dsp_fw::SchedulersInfo> schedulers;

    CavsTopologySample::createInstanceFirmwareObjects(moduleInstances, gateways, pipelineIds,
                                                      pipelines, schedulers);

    /* Gateways*/
    commands.addGetGatewaysCommand(true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                   static_cast<uint32_t>(gateways.size()), gateways);

    /* Pipelines*/
    commands.addGetPipelineListCommand(true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                       static_cast<uint32_t>(CavsTopologySample::maxPplCount),
                                       pipelineIds);

    for (auto &pipeline : pipelines) {
        commands.addGetPipelinePropsCommand(true, STATUS_SUCCESS,
                                            dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                            dsp_fw::PipeLineIdType{pipeline.id}, pipeline);
    }

    /* Schedulers */
    uint32_t coreId = 0;
    for (auto &scheduler : schedulers) {
        commands.addGetSchedulersInfoCommand(true, STATUS_SUCCESS,
                                             dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                             dsp_fw::CoreId{coreId++}, scheduler);
    }

    /* Module instances */
    for (auto &module : moduleInstances) {

        commands.addGetModuleInstancePropsCommand(true, STATUS_SUCCESS,
                                                  dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                                  module.id.moduleId, module.id.instanceId, module);
    }
}

static void requestInstanceTopologyRefresh(HttpClientSimulator &client)
{
    client.request("/instance/cavs/0/refreshed", HttpClientSimulator::Verb::Post, "",
                   HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent(""));
}

/* Check that urls contained in the supplied map match the expected xml result
 *
 * Key of the map: the url
 * Value of the map: a file that contains the expected xml
 */
static void checkUrlMap(HttpClientSimulator &client,
                        const std::map<std::string, std::string> &urlMap)
{
    for (auto it : urlMap) {
        try {
            client.request(it.first, HttpClientSimulator::Verb::Get, "",
                           HttpClientSimulator::Status::Ok, "text/xml",
                           HttpClientSimulator::FileContent(xmlFileName(it.second)));
        } catch (std::exception &e) {
            std::stringstream stream;
            stream << "Error on url=" << it.first << " file=" << it.second << std::endl << e.what();
            INFO(stream.str());
            CHECK(false);
        }
    }
}

struct Fixture
{
    // When the mocked device is destructed, check that all inputs were
    // consumed.
    std::unique_ptr<windows::MockedDevice> device{
        new windows::MockedDevice([] { INFO("There are leftover test inputs."; CHECK(false);) })};
};

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: topology")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* System type and instance are available before refresh */
    std::map<std::string, std::string> systemUrlMap = {
        {"/type", "windows/system_type"}, {"/instance", "windows/system_instance"},
    };
    checkUrlMap(client, systemUrlMap);

    /* Request an instance topology refresh */
    CHECK_NOTHROW(requestInstanceTopologyRefresh(client));

    /* Testing urls that depend of topology retrieval */
    std::map<std::string, std::string> urlMap = {
        {"/type/cavs", "subsystem_type"},
        {"/instance/cavs", "windows/subsystem_instance_collection"},
        {"/instance/cavs/0", "windows/subsystem_instance"},

        {"/type/cavs.pipe", "pipe_type"},
        {"/instance/cavs.pipe", "pipe_instance_collection"},
        {"/instance/cavs.pipe/1", "pipe_instance"},

        {"/type/cavs.task", "task_type"},
        {"/instance/cavs.task", "task_instance_collection"},
        {"/instance/cavs.task/1", "task_instance"},

        {"/type/cavs.core", "core_type"},
        {"/instance/cavs.core", "core_instance_collection"},
        {"/instance/cavs.core/0", "core_instance"},

        {"/type/cavs.module-aec", "module_type"},
        {"/instance/cavs.module-aec", "module_instance_collection"},
        {"/instance/cavs.module-aec/2", "module_instance"},

        {"/type/cavs.hda-host-out-gateway", "gateway_type"},
        {"/instance/cavs.hda-host-out-gateway", "gateway_instance_collection"},
        {"/instance/cavs.hda-host-out-gateway/1", "gateway_instance"},

        {"/type/cavs.fwlogs", "logservice_type"},
        {"/instance/cavs.fwlogs", "logservice_instance_collection"},
        {"/instance/cavs.fwlogs/0", "logservice_instance"},

        {"/type/cavs.probe", "probeservice_type"},
        {"/instance/cavs.probe", "probeservice_instance_collection"},
        {"/instance/cavs.probe/0", "probeservice_instance"},

        {"/type/cavs.perf_measurement", "perf_measurementservice_type"},
        {"/instance/cavs.perf_measurement", "perf_measurementservice_instance_collection"},
        {"/instance/cavs.perf_measurement/0", "perf_measurementservice_instance"},

        {"/type/cavs.probe.endpoint", "probeservice_endpoint_type"},
        {"/instance/cavs.probe.endpoint", "probeservice_endpoint_instance_collection"},
        {"/instance/cavs.probe.endpoint/0", "probeservice_endpoint_instance"},
    };

    checkUrlMap(client, urlMap);
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: internal debug urls")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    CHECK_NOTHROW(client.request(
        "/internal/modules", HttpClientSimulator::Verb::Get, "", HttpClientSimulator::Status::Ok,
        "text/html", HttpClientSimulator::FileContent(htmlFileName("internal_module_list"))));

    CHECK_NOTHROW(client.request(
        "/internal/topology", HttpClientSimulator::Verb::Get, "", HttpClientSimulator::Status::Ok,
        "text/html", HttpClientSimulator::FileContent(htmlFileName("internal_topology"))));
}

static const dsp_fw::ParameterId AecParameterId{0};

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: GET module instance control parameters "
                          "(URL: /instance/cavs.module-aec/1/control_parameters)")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);

        /* Add command for get module parameter */
        uint16_t moduleId = 1;
        uint16_t InstanceId = 1;
        commands.addGetModuleParameterCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, moduleId, InstanceId,
            AecParameterId, aecControlParameterPayload);
        commands.addGetModuleParameterCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, moduleId, InstanceId,
            dsp_fw::ParameterId{25}, nsControlParameterPayload);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* Request an instance topology refresh */
    CHECK_NOTHROW(requestInstanceTopologyRefresh(client));

    /* 1: Getting system information*/
    CHECK_NOTHROW(client.request(
        "/instance/cavs.module-aec/1/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("module_instance_control_params"))));
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: A refresh error erases the previous topology ")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);

        /* Add a bad gateway command */
        std::vector<dsp_fw::GatewayProps> emptyList;
        commands.addGetGatewaysCommand(false, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                                       static_cast<uint32_t>(CavsTopologySample::gatewaysCount),
                                       emptyList);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* Request an instance topology refresh */
    CHECK_NOTHROW(requestInstanceTopologyRefresh(client));

    /* Access to an instance */
    CHECK_NOTHROW(client.request("/instance/cavs.module-aec/2", HttpClientSimulator::Verb::Get, "",
                                 HttpClientSimulator::Status::Ok, "text/xml",
                                 HttpClientSimulator::FileContent(xmlFileName("module_instance"))));

    /* Request an instance topology refresh which will fail since commands are not in device mock */
    CHECK_NOTHROW(client.request(
        "/instance/cavs/0/refreshed", HttpClientSimulator::Verb::Post, "",
        HttpClientSimulator::Status::InternalError, "text/plain",
        HttpClientSimulator::StringContent(
            "Internal error: Cannot refresh instance model: Cannot get topology from fw: "
            "Can not retrieve gateways: Device returns an exception: OS says that io "
            "control has failed.")));

    /* Access to an instance must fail since last topology refresh has failed*/
    CHECK_NOTHROW(client.request(
        "/instance/cavs.module-aec/2", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::InternalError, "text/plain",
        HttpClientSimulator::StringContent("Internal error: Instance model is undefined.")));
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: Set module instance control parameters "
                          "(URL: /instance/cavs.module-aec/1/control_parameters)")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);

        uint16_t moduleId = 1;
        uint16_t InstanceId = 1;
        commands.addSetModuleParameterCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, moduleId, InstanceId,
            AecParameterId, aecControlParameterPayload);

        commands.addSetModuleParameterCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, moduleId, InstanceId,
            dsp_fw::ParameterId{25}, nsControlParameterPayload);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* Request an instance topology refresh */
    CHECK_NOTHROW(requestInstanceTopologyRefresh(client));

    CHECK_NOTHROW(client.request(
        "/instance/cavs.module-aec/1/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("module_instance_control_params")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));
}

TEST_CASE_METHOD(Fixture, "DebugAgent / cAVS: Getting structure of parameters(module, logs)")
{
    /* Setting the test vector
     * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* Checking structure answers */
    std::map<std::string, std::string> systemUrlMap = {
        {"/type/cavs.module-aec/control_parameters", "module_type_control_params"}, // module
        {"/type/cavs.fwlogs/control_parameters", "logservice_control_parameter_structure"}, // logs
    };
    checkUrlMap(client, systemUrlMap);
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: log parameters (URL: /instance/cavs.fwlogs/0)")
{
    /* Setting the test vector
    * ----------------------- */

    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* 1: Get log parameter, will return
        * - isStarted : false
        * - level: critical
        * - output: pti
        */

        windows::driver::IoctlFwLogsState initialLogParams = {
            windows::driver::IOCTL_LOG_STATE::STOPPED, windows::driver::FW_LOG_LEVEL::LOG_CRITICAL,
            windows::driver::FW_LOG_OUTPUT::OUTPUT_PTI};
        commands.addGetLogParametersCommand(true, STATUS_SUCCESS, initialLogParams);

        /* 2: Set log parameter to
        * - isStarted : true
        * - level: verbose
        * - output: sram
        */
        windows::driver::IoctlFwLogsState setLogParams = {
            windows::driver::IOCTL_LOG_STATE::STARTED, windows::driver::FW_LOG_LEVEL::LOG_VERBOSE,
            windows::driver::FW_LOG_OUTPUT::OUTPUT_WPP};
        commands.addSetLogParametersCommand(true, STATUS_SUCCESS, setLogParams);

        /* 3: Get log parameter , will return
        * - isStarted : true
        * - level: verbose
        * - output: sram
        */
        commands.addGetLogParametersCommand(true, STATUS_SUCCESS, setLogParams);

        /** Adding a successful set log parameters command, this is called by the System class
        * destructor to stop log */
        setLogParams = {windows::driver::IOCTL_LOG_STATE::STOPPED,
                        windows::driver::FW_LOG_LEVEL::LOG_VERBOSE,
                        windows::driver::FW_LOG_OUTPUT::OUTPUT_WPP};
        commands.addSetLogParametersCommand(true, STATUS_SUCCESS, setLogParams);
    }

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* 1: Getting log parameters*/
    CHECK_NOTHROW(client.request(
        "/instance/cavs.fwlogs/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("logservice_getparam_stopped"))));

    /* 2: Setting log parameters ("1;Verbose;SRAM") */
    CHECK_NOTHROW(client.request(
        "/instance/cavs.fwlogs/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("logservice_setparam_start")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    /* 3: Getting log parameters again */
    CHECK_NOTHROW(client.request(
        "/instance/cavs.fwlogs/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("logservice_getparam_started"))));
}

// Probing constants
static const std::size_t probeCount = 8;
static const std::size_t injectionProbeIndex = 0;
static const std::size_t extractionProbeIndex = 1;
static_assert(extractionProbeIndex < probeCount, "wrong extraction probe index");
static_assert(injectionProbeIndex < probeCount, "wrong injection probe index");

static const dsp_fw::BitDepth bitDepth = dsp_fw::BitDepth::DEPTH_16BIT;
static const std::size_t channelCount = 4;
static const std::size_t sampleByteSize = (dsp_fw::BitDepth::DEPTH_16BIT / 8) * channelCount;
static const std::size_t ringBufferSize = 31;

// To test corner cases better
static_assert(ringBufferSize % sampleByteSize != 0, "Ring buffer size shall not be aligned "
                                                    "with sample byte size");

/**
 * Generate a buffer filled with 20 probe extraction packets. Packet size is deduced from
 * the index [0..20[
 */
Buffer generateProbeExtractionContent(dsp_fw::ProbePointId probePointId)
{
    MemoryByteStreamWriter writer;
    for (uint32_t i = 0; i < 20; i++) {
        dsp_fw::Packet packet{};
        packet.probePointId = probePointId;

        // using index as size and byte value
        packet.data.resize(i, i);
        writer.write(packet);
    }
    return writer.getBuffer();
}

/**
 * Split a buffer into chunks.
 * Each chunk size is deduced from a size list, using this formula:
 * chunk_size[i] = sizeList[ i % sizeList.size() ]
 */
std::vector<Buffer> splitBuffer(const util::Buffer &buffer, std::vector<std::size_t> sizeList)
{
    std::size_t current = 0;
    std::size_t i = 0;
    std::vector<Buffer> buffers;
    while (current < buffer.size()) {
        std::size_t chunkSize = std::min(sizeList[i % sizeList.size()], buffer.size() - current);
        buffers.emplace_back(buffer.begin() + current, buffer.begin() + current + chunkSize);
        current += chunkSize;
        ++i;
    }
    return buffers;
}

/** Simulate driver probe extraction ring buffer */
class FakeRingBuffer
{
public:
    /** Note: buffer is initialized with 0xFF values */
    FakeRingBuffer(std::size_t size) : mBuffer(size, 0xFF), mLinearOffset(0) {}

    /** Write content in the ring buffer */
    void write(const Buffer &content)
    {
        std::size_t arrayOffset = mLinearOffset % mBuffer.size();

        ASSERT_ALWAYS(content.size() <= mBuffer.size());
        if (arrayOffset + content.size() <= mBuffer.size()) {
            std::copy(content.begin(), content.end(), mBuffer.begin() + arrayOffset);
        } else {
            std::size_t firstPartSize = mBuffer.size() - arrayOffset;

            std::copy_n(content.begin(), firstPartSize, mBuffer.begin() + arrayOffset);
            std::copy(content.begin() + firstPartSize, content.end(), mBuffer.begin());
        }
        mLinearOffset += content.size();
    }

    uint8_t *getBufferPtr() { return mBuffer.data(); }
    std::size_t getBufferSize() { return mBuffer.size(); }

    const Buffer &getBuffer() { return mBuffer; }
    uint64_t getProducerPosition() { return mLinearOffset; }

private:
    Buffer mBuffer;
    uint64_t mLinearOffset;
};

/** Create data for probe injection */
Buffer createInjectionData()
{
    static const std::size_t sampleCount = 100;
    static std::size_t byteCount = sampleCount * sampleByteSize;
    Buffer buffer(byteCount);
    for (std::size_t i = 0; i < byteCount; i++) {
        buffer[i] = i % 256;
    }
    return buffer;
}

/** Create injection expected ring buffer content and consumer position list from injected data.
 *
 * @param[in] data injected data
 * @param[out] consumerPositions list of consumer (=the driver) positions
 * @return the list of expected ring buffers
 */
std::vector<Buffer> createExpectedInjectionBuffers(const util::Buffer &data,
                                                   std::vector<std::size_t> &consumerPositions)
{
    /** Consumer position will be incremented by this value */
    static const std::size_t consumerPositionDelta = 21;
    static_assert(consumerPositionDelta % sampleByteSize != 0,
                  "consumerPositionDelta shall "
                  "not be a multiple of sampleByteSize");

    static const std::size_t ringBufferSampleCount = ringBufferSize / sampleByteSize;

    std::vector<Buffer> buffers;
    std::size_t consumerPosition = 0;
    FakeRingBuffer ringBuffer(ringBufferSize);
    MemoryInputStream is(data);

    // Prefilling buffer with silence
    util::Buffer block(ringBufferSampleCount * sampleByteSize, 0);
    ringBuffer.write(block);
    buffers.push_back(ringBuffer.getBuffer());

    // Simulating consumer update
    consumerPosition += consumerPositionDelta;
    consumerPositions.push_back(consumerPosition);

    // Then filling buffer from injected data
    while (!is.isEOS()) {

        // calculating next block size
        std::size_t availableBytes =
            ringBuffer.getBufferSize() - (ringBuffer.getProducerPosition() - consumerPosition);
        std::size_t availableSamples = availableBytes / sampleByteSize;
        std::size_t availableSampleBytes = availableSamples * sampleByteSize;
        block.resize(availableSampleBytes);

        // Reading injected data
        auto read = is.read(block.data(), block.size());
        if (read < block.size()) {

            // Completing with silence if needed
            std::fill(block.begin() + read, block.end(), 0);
        }

        // Writing to the ring buffer
        ringBuffer.write(block);

        // Saving buffer
        buffers.push_back(ringBuffer.getBuffer());

        // Simulating consumer update
        consumerPosition += consumerPositionDelta;
        consumerPositions.push_back(consumerPosition);
    };

    return buffers;
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: probe service control nominal cases")
{
    /* Setting the test vector
     * ----------------------- */

    auto probeEventHandles =
        windows::ProberBackend::EventHandlesFactory<windows::TestEventHandle>::createHandles();

    // Getting a reference to the extraction event handle, in order to simulate driver
    // behaviour */
    auto &extractionHandle =
        dynamic_cast<windows::TestEventHandle &>(*probeEventHandles.extractionHandle);
    auto &injectionHandle =
        dynamic_cast<windows::TestEventHandle &>(*probeEventHandles.injectionHandles[0]);

    // Generating probe extraction content
    const dsp_fw::ProbePointId probePointId(1, 2, dsp_fw::ProbeType::Output, 0);
    Buffer extractedContent = generateProbeExtractionContent(probePointId);

    // Splitting extraction content in blocks : each block will be written to the ring buffer
    // Block sizes are {1, 10, 20, 30} (cycling)
    auto blocks = splitBuffer(extractedContent, {1, 10, 20, 30});

    // Creating the fake ring buffer, with a size that matches the max block size, in order
    // to test the case (written data size == ring buffer size)
    FakeRingBuffer extractionBuffer(ringBufferSize);

    // Creating the fake injection buffer
    Buffer injectionBuffer(ringBufferSize, 0xFF);

    // Creating injection data
    Buffer injectData = createInjectionData();

    // Creating expected injection blocks and consumer positions
    std::vector<size_t> consumerPositions;
    auto expectedInjectionBlocks = createExpectedInjectionBuffers(injectData, consumerPositions);

    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        // 1 : Getting probe service parameters, checking that it is stopped
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);

        // 2 : Getting probe endpoint parameters, checking that they are deactivated
        // -> involves no ioctl

        // 3 : Configuring probe #0 to be enabled for injection and probe #1 to be enabled for
        //     extraction
        // -> involves no ioctl

        // 4 : Getting probe endpoint parameters, checking that they are deactivated except the one
        //     that has been enabled
        // -> involves no ioctl

        // 5 : Starting service
        // starting the service with enabled injection probe involves to retrieve a module instance
        // props in order tocalculate sample byte size. So initializing bit depth and channel count
        // of the module instance pin used for injection
        dsp_fw::PinProps pinProps{};
        pinProps.format.bit_depth = bitDepth;
        pinProps.format.number_of_channels = channelCount;

        dsp_fw::ModuleInstanceProps moduleInstanceProps{};
        moduleInstanceProps.input_pins.pin_info.push_back(pinProps);

        commands.addGetModuleInstancePropsCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, 1, 2, moduleInstanceProps);

        // getting current state : idle
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);

        // going to Owned
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureOwned);

        using Type = dsp_fw::ProbeType;
        using Purpose = Prober::ProbePurpose;
        // setting probe configuration (probe #1 is enabled)
        cavs::Prober::SessionProbes probes = {{true, {1, 2, Type::Input, 0}, Purpose::Inject},
                                              {true, {1, 2, Type::Output, 0}, Purpose::Extract},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject}};

        commands.addSetProbeConfigurationCommand(
            true, STATUS_SUCCESS, windows::ProberBackend::toWindows(probes, probeEventHandles));

        // going to Allocated
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureAllocated);

        // going to Get ring buffers
        windows::driver::RingBuffersDescription rb = {
            {extractionBuffer.getBufferPtr(), extractionBuffer.getBufferSize()},
            {{injectionBuffer.data(), injectionBuffer.size()}}};
        commands.addGetRingBuffers(true, STATUS_SUCCESS, rb);

        // going to Active
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureActive);

        // 6 : Getting probe service parameters, checking that it is started
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureActive);

        // 7 : Extract from an enabled probe

        // Adding a "get extraction linear position" command for each block written by the driver
        uint64_t linearPosition = 0;
        for (auto &block : blocks) {
            linearPosition += block.size();
            commands.addGetExtractionRingBufferLinearPosition(true, STATUS_SUCCESS, linearPosition);
        }

        // Adding a "get injection linear position" command for each block read by the driver
        for (auto consumerPos : consumerPositions) {
            commands.addGetInjectionRingBufferLinearPosition(true, STATUS_SUCCESS, 0, consumerPos);
        }

        // 8 : Stopping service

        // getting current state : Active
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureActive);

        // going to Allocated, Owned and Idle
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureAllocated);
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureOwned);
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);

        // 9: Getting probe service parameters, checking that it is stopped
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);
    }

    /* Now using the mocked device
     * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(), probeEventHandles);

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    // 1 : Getting probe service parameters, checking that it is stopped
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("probeservice_param_stopped"))));

    // 2 : Getting probe endpoint parameters, checking that they are deactivated
    for (std::size_t probeIndex = 0; probeIndex < probeCount; ++probeIndex) {
        CHECK_NOTHROW(client.request(
            "/instance/cavs.probe.endpoint/" + std::to_string(probeIndex) + "/control_parameters",
            HttpClientSimulator::Verb::Get, "", HttpClientSimulator::Status::Ok, "text/xml",
            HttpClientSimulator::FileContent(xmlFileName("probeservice_endpoint_param_disabled"))));
    }

    // 3 : Configuring probe #0 to be enabled for injection and probe #1 to be enabled for
    //     extraction
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe.endpoint/" + std::to_string(extractionProbeIndex) +
            "/control_parameters",
        HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("probeservice_endpoint_param_enabled_extract")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe.endpoint/" + std::to_string(injectionProbeIndex) +
            "/control_parameters",
        HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("probeservice_endpoint_param_enabled_inject")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    // 4 : Getting probe endpoint parameters, checking that they are deactivated except the one that
    //     has been enabled
    for (std::size_t probeIndex = 0; probeIndex < probeCount; ++probeIndex) {
        std::string expectedFile;
        if (probeIndex == extractionProbeIndex) {
            expectedFile = "probeservice_endpoint_param_enabled_extract";
        } else if (probeIndex == injectionProbeIndex) {
            expectedFile = "probeservice_endpoint_param_enabled_inject";
        } else {
            expectedFile = "probeservice_endpoint_param_disabled";
        }

        CHECK_NOTHROW(client.request(
            "/instance/cavs.probe.endpoint/" + std::to_string(probeIndex) + "/control_parameters",
            HttpClientSimulator::Verb::Get, "", HttpClientSimulator::Status::Ok, "text/xml",
            HttpClientSimulator::FileContent(xmlFileName(expectedFile))));
    }

    // 5 : Starting service
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("probeservice_param_started")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    // 6 : Getting probe service parameters, checking that it is started
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("probeservice_param_started"))));

    // 7 : Extract from an enabled probe. TODO: currently, extraction is not implemented and the
    // result will be empty.
    auto future = std::async(std::launch::async, [&] {
        client.request("/instance/cavs.probe.endpoint/" + std::to_string(extractionProbeIndex) +
                           "/streaming",
                       HttpClientSimulator::Verb::Get, "", HttpClientSimulator::Status::Ok,
                       "application/vnd.ifdk-file",
                       HttpClientSimulator::FileContent(dataPath + "probe_1_extraction.bin"));
    });

    // simulating the driver extraction by filling the ring buffer
    for (auto &block : blocks) {
        // waiting until the extraction thread is idle
        extractionHandle.blockUntilWait();

        // writing the block in the ring buffer
        extractionBuffer.write(block);

        // notify the DBGA that the content has been written
        extractionHandle.notify();
    }

    // Waiting until the extraction thread is idle to prevent of races with injection thread
    extractionHandle.blockUntilWait();

    // Sending inject data to the DBGA
    // Prefix the inject data with the probe header
    std::ostringstream oss;
    oss << system::IfdkStreamHeader("generic", "probe", 1, 0);
    std::string injectionHeader(oss.str());
    injectData.insert(begin(injectData), begin(injectionHeader), end(injectionHeader));

    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe.endpoint/" + std::to_string(injectionProbeIndex) + "/streaming",
        HttpClientSimulator::Verb::Put, injectData, HttpClientSimulator::Status::Ok, "",
        HttpClientSimulator::StringContent("")));

    // Simulating the driver injection by checking the ring buffer content
    for (auto &expectedBlock : expectedInjectionBlocks) {
        // waiting until the inkection thread is idle
        injectionHandle.blockUntilWait();

        // Checking that ring buffer content is the expected one
        REQUIRE(injectionBuffer == expectedBlock);

        // notify the DBGA that the content has been read
        injectionHandle.notify();
    }

    injectionHandle.blockUntilWait();

    // 8 : Stopping service
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("probeservice_param_stopped")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    // Checking that step 7 has not failed
    CHECK_NOTHROW(future.get());

    // 9: Getting probe service parameters, checking that it is stopped
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("probeservice_param_stopped"))));
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: probe service control failure cases")
{
    /* Setting the test vector
    * ----------------------- */

    auto &&probeEventHandles = windows::ProberBackend::SystemEventHandlesFactory::createHandles();
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        // 1 : Getting probe service state, with an inconsistent driver state (Owned)
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureOwned);

        // 2 : If service starting fails, it should come back to "Idle" state

        // going to Owned state and setting configuration
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureOwned);

        using Type = dsp_fw::ProbeType;
        using Purpose = Prober::ProbePurpose;
        cavs::Prober::SessionProbes probes = {{false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject},
                                              {false, {0, 0, Type::Input, 0}, Purpose::Inject}};

        commands.addSetProbeConfigurationCommand(
            true, STATUS_SUCCESS, windows::ProberBackend::toWindows(probes, probeEventHandles));

        // going to Allocated, but the it fails!
        commands.addSetProbeStateCommand(false, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureAllocated);

        // coming back to idle : firstly getting current state (Owned), then going to Idle state
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureOwned);
        commands.addSetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);

        // 3 : getting state: should be Idle
        commands.addGetProbeStateCommand(true, STATUS_SUCCESS,
                                         windows::driver::ProbeState::ProbeFeatureIdle);
    }

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(), probeEventHandles);

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    // 1 : Getting probe service state, with an inconsistent driver state (Owned)
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::InternalError, "text/plain",
        HttpClientSimulator::StringContent(
            "Internal error: ParameterDispatcher: cannot get "
            "parameter value: Cannot get probe service state: Cannot get probe service state: "
            "Unexpected driver probe service state: Owned "
            "(type=cavs.probe kind=Control instance=0)")));

    // 2 : If service starting fails, it should come back to "Idle" state
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("probeservice_param_started")),
        HttpClientSimulator::Status::InternalError, "text/plain",
        HttpClientSimulator::StringContent(
            "Internal error: ParameterDispatcher: cannot set "
            "control parameter value: Unable to set probe service state: Cannot set probe service "
            "state: Unable to set state to driver: TinySet error: OS says that io control has "
            "failed. (type=cavs.probe kind=Control instance=0\nvalue:\n"
            "<control_parameters>\n"
            "    <ParameterBlock Name=\"State\">\n"
            "        <BooleanParameter Name=\"Started\">1</BooleanParameter>\n"
            "    </ParameterBlock>\n"
            "</control_parameters>\n\n)")));

    // 3 : getting state: should be Idle
    CHECK_NOTHROW(client.request(
        "/instance/cavs.probe/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("probeservice_param_stopped"))));
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: performance measurement", "[perf]")
{
    /* Setting the test vector
    * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        // 1 Starting
        commands.addSetPerfState(true, STATUS_SUCCESS, Perf::State::Started);

        // 2 Check that "get state" returns "started"
        commands.addGetPerfState(true, STATUS_SUCCESS, Perf::State::Started);

        // 3 Paused
        commands.addSetPerfState(true, STATUS_SUCCESS, Perf::State::Paused);

        // 4 Stop
        commands.addSetPerfState(true, STATUS_SUCCESS, Perf::State::Stopped);
    }

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    // 1 Starting
    CHECK_NOTHROW(client.request(
        "/instance/cavs.perf_measurement/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("perfservice_started")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    // 2 Check that "get state" returns "started"
    CHECK_NOTHROW(client.request(
        "/instance/cavs.perf_measurement/0/control_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("perfservice_started"))));

    // 3 Paused
    CHECK_NOTHROW(client.request(
        "/instance/cavs.perf_measurement/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("perfservice_paused")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    // 4 Stopped
    CHECK_NOTHROW(client.request(
        "/instance/cavs.perf_measurement/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("perfservice_stopped")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));
}

TEST_CASE_METHOD(Fixture, "DebugAgent/cAVS: subsystem info parameters", "[subsystem],[info]")
{
    /* Setting the test vector
    * ----------------------- */
    {
        windows::MockedDeviceCommands commands(*device);
        DBGACommandScope scope(commands);

        /* Adding topology command */
        addInstanceTopologyCommands(commands);

        // 1 Starting
        commands.addSetPerfState(true, STATUS_SUCCESS, Perf::State::Started);

        // perf items ioctl
        static const std::vector<dsp_fw::PerfDataItem> returnedItems = {
            dsp_fw::PerfDataItem(0, 0, false, false, 1337, 42),   // Core 0
            dsp_fw::PerfDataItem(1, 0, true, false, 123456, 789), // Module 1, instance 0
            dsp_fw::PerfDataItem(9, 0, false, false, 1111, 222),  // Module 9, instance 0
            dsp_fw::PerfDataItem(9, 1, true, false, 3333, 444),   // Module 9, instance 1
            dsp_fw::PerfDataItem(9, 2, true, true, 5555, 666)     // Module 9, instance 2, removed
        };
        commands.addGetPerfItems(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
            CavsTopologySample::maxModInstCount + CavsTopologySample::dspCoreCount, returnedItems);

        // 1st module instance props ioctl
        {
            dsp_fw::ModuleInstanceProps moduleInstanceProps{};
            moduleInstanceProps.id = {1, 0};
            moduleInstanceProps.ibs_bytes = 1;
            moduleInstanceProps.cpc = 2000;

            dsp_fw::PinProps pinProps{};
            pinProps.format.sampling_frequency = dsp_fw::SamplingFrequency::FS_8000HZ;
            pinProps.format.number_of_channels = 4;
            pinProps.format.bit_depth = dsp_fw::BitDepth::DEPTH_8BIT;
            pinProps.format.valid_bit_depth = 8;

            moduleInstanceProps.input_pins.pin_info.push_back(pinProps);
            commands.addGetModuleInstancePropsCommand(
                true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                moduleInstanceProps.id.moduleId, moduleInstanceProps.id.instanceId,
                moduleInstanceProps);
        }

        // 2nd module instance props ioctl
        {
            dsp_fw::ModuleInstanceProps moduleInstanceProps{};
            moduleInstanceProps.id = {9, 0};
            moduleInstanceProps.ibs_bytes = 9;
            moduleInstanceProps.cpc = 10000;

            dsp_fw::PinProps pinProps{};
            pinProps.format.sampling_frequency = dsp_fw::SamplingFrequency::FS_11025HZ;
            pinProps.format.number_of_channels = 12;
            pinProps.format.bit_depth = dsp_fw::BitDepth::DEPTH_16BIT;
            pinProps.format.valid_bit_depth = 16;

            moduleInstanceProps.input_pins.pin_info.push_back(pinProps);
            commands.addGetModuleInstancePropsCommand(
                true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                moduleInstanceProps.id.moduleId, moduleInstanceProps.id.instanceId,
                moduleInstanceProps);
        }

        // 3rd module instance props ioctl
        {
            dsp_fw::ModuleInstanceProps moduleInstanceProps{};
            moduleInstanceProps.id = {9, 1};
            moduleInstanceProps.ibs_bytes = 17;
            moduleInstanceProps.cpc = 18000;

            dsp_fw::PinProps pinProps{};
            pinProps.format.sampling_frequency = dsp_fw::SamplingFrequency::FS_12000HZ;
            pinProps.format.number_of_channels = 20;
            pinProps.format.bit_depth = dsp_fw::BitDepth::DEPTH_32BIT;
            pinProps.format.valid_bit_depth = 24;

            moduleInstanceProps.input_pins.pin_info.push_back(pinProps);
            commands.addGetModuleInstancePropsCommand(
                true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS,
                moduleInstanceProps.id.moduleId, moduleInstanceProps.id.instanceId,
                moduleInstanceProps);
        }

        Buffer globalMemoryState = {/* Tag for LPSRAM_STATE: 0 */
                                    0, 0, 0, 0,
                                    /* Length: (1+(1+1)+(1+0.5)) = 4.5 */
                                    18, 0, 0, 0,
                                    /* Value */
                                    /* Free physical memory pages (42) */
                                    42, 0, 0, 0,
                                    /* number of EBB state entries */
                                    1, 0, 0, 0,
                                    /* EBB state */
                                    0x01, 0, 0, 0,
                                    /* number of page alloc entries */
                                    1, 0, 0, 0,
                                    /* page alloc */
                                    42, 0x00,
                                    /* Tag for HPSRAM_STATE: 1 */
                                    1, 0, 0, 0,
                                    /* Length: (1+(1+2) + (1+1.5)) * 6.5 */
                                    26, 0, 0, 0,
                                    /* Value */
                                    /* Free physical memory pages (1337 = 0x0539) */
                                    0x39, 0x05, 0, 0,
                                    /* number of EBB state entries */
                                    2, 0, 0, 0,
                                    /* EBB state, part 1 */
                                    0xff, 0x00, 0x00, 0x00,
                                    /* EBB state, part 2 */
                                    0xff, 0xff, 0xff, 0xff,
                                    /* number of page alloc entries */
                                    3, 0, 0, 0,
                                    /* page alloc, parts 1, 2 & 3 */
                                    42, 0x00, 0x00, 0x00, 0x00, 0xff};

        /* Add command for get module parameter */
        commands.addGetGlobalMemoryStateCommand(
            true, STATUS_SUCCESS, dsp_fw::IxcStatus::ADSP_IPC_SUCCESS, globalMemoryState);
    }

    /* Now using the mocked device
    * --------------------------- */

    /* Creating the factory that will inject the mocked device */
    windows::DeviceInjectionDriverFactory driverFactory(
        std::move(device), std::make_unique<windows::StubbedWppClientFactory>(),
        windows::ProberBackend::SystemEventHandlesFactory::createHandles());

    /* Creating and starting the debug agent */
    DebugAgent debugAgent(driverFactory, HttpClientSimulator::DefaultPort, pfwConfigPath);

    /* Creating the http client */
    HttpClientSimulator client("localhost");

    /* Request an instance topology refresh */
    CHECK_NOTHROW(requestInstanceTopologyRefresh(client));

    /* start the perf service */
    CHECK_NOTHROW(client.request(
        "/instance/cavs.perf_measurement/0/control_parameters", HttpClientSimulator::Verb::Put,
        file_helper::readAsString(xmlFileName("perfservice_started")),
        HttpClientSimulator::Status::Ok, "", HttpClientSimulator::StringContent("")));

    /* 1: Getting system information*/
    CHECK_NOTHROW(client.request(
        "/instance/cavs/0/info_parameters", HttpClientSimulator::Verb::Get, "",
        HttpClientSimulator::Status::Ok, "text/xml",
        HttpClientSimulator::FileContent(xmlFileName("subsystem_instance_info_params"))));
}
