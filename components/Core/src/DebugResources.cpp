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

#include "HtmlHelper.hpp"
#include "FdkToolMockGenerator.hpp"
#include "Core/DebugResources.hpp"
#include "Rest/StreamResponse.hpp"
#include "cAVS/Topology.hpp"
#include "Util/StringHelper.hpp"
#include "Util/Uuid.hpp"
#include "Util/About.hpp"
#include <Poco/Zip/Compress.h>
#include <Poco/Zip/ZipException.h>
#include <sstream>
#include <vector>
#include <iomanip>

using namespace debug_agent::rest;
using namespace debug_agent::cavs;
using namespace debug_agent::util;
using namespace debug_agent::ifdk_objects;

namespace debug_agent
{
namespace core
{

static const std::string ContentTypeHtml("text/html");
static const std::string ContentTypeZip("application/zip");

/** Helper method to convert an hash array into string */
std::string hashToString(const decltype(dsp_fw::ModuleEntry::hash) hash)
{
    std::stringstream stream;

    /* Common settings*/
    stream << std::setfill('0') << std::hex << std::uppercase;

    /* Each byte */
    for (const auto byte : hash) {
        stream << std::setw(2) << static_cast<uint32_t>(byte);
    }

    return stream.str();
}

/** Helper method to convert an simple type array into string, by separating elements by
 * a comma */
template <typename T>
std::string vectorToString(const std::vector<T> &vector)
{
    std::stringstream stream;
    for (auto &value : vector) {
        stream << value;
        stream << ", ";
    }
    return stream.str();
}

/** Helper method that converts a gateway into string, under the form:
 * - if the value is valid : ( <gateway type name>(gateway type id), <instance id>)
 *    for instance: ( my_gateway(2), 5)
 * - if the value is undefined: return value is "none"
 */
std::string gatewayToString(const dsp_fw::ConnectorNodeId &connector)
{
    if (connector.val.dw == dsp_fw::ConnectorNodeId::kInvalidNodeId) {
        return "none";
    }

    dsp_fw::ConnectorNodeId::Type type =
        static_cast<dsp_fw::ConnectorNodeId::Type>(connector.val.f.dma_type);

    std::stringstream stream;
    stream << "(" << dsp_fw::ConnectorNodeId::getTypeEnumHelper().toString(type) << "("
           << connector.val.f.dma_type << "), " << connector.val.f.v_index << ")";

    return stream.str();
}

Resource::ResponsePtr ModuleListDebugResource::handleGet(const Request &)
{
    /* Segment count per module entry, as defined in firmware structures */
    static const std::size_t segmentCount = 3;

    /* Retrieving module entries, doesn't throw exception */
    const std::vector<dsp_fw::ModuleEntry> &entries = mSystem.getModuleHandler().getModuleEntries();

    HtmlHelper html;
    html.title("Module type list");
    html.paragraph("Module count: " + std::to_string(entries.size()));

    /* Module list column names */
    std::vector<std::string> columns = {
        "module_id",          "name",      "uuid",          "module_id",
        "state_flags",        "type",      "hash",          "entry_point",
        "cfg_offset",         "cfg_count", "affinity_mask", "intance_max_count",
        "instance_stack_size"};

    /* Adding segment column names */
    for (std::size_t i = 0; i < segmentCount; ++i) {
        const std::string segmentName("segments[" + std::to_string(i) + "]");
        columns.push_back(segmentName + ".flags");
        columns.push_back(segmentName + ".v_base_addr");
        columns.push_back(segmentName + ".file_offset");
    }

    html.beginTable(columns);

    for (auto &entry : entries) {
        html.beginRow();

        html.cell(entry.module_id);
        html.cell(entry.getName());

        util::Uuid uuid;
        uuid.fromOtherUuidType(entry.uuid);
        html.cell(uuid.toString());

        html.cell(entry.module_id);
        html.cell(entry.state_flags);
        html.cell(entry.type.ul);
        html.cell(hashToString(entry.hash));
        html.cell(entry.entry_point);
        html.cell(entry.cfg_offset);
        html.cell(entry.cfg_count);
        html.cell(entry.affinity_mask);
        html.cell(entry.instance_max_count);
        html.cell(entry.instance_stack_size);

        for (std::size_t i = 0; i < segmentCount; ++i) {
            html.cell(entry.segments[i].flags.ul);
            html.cell(entry.segments[i].v_base_addr);
            html.cell(entry.segments[i].file_offset);
        }

        html.endRow();
    }

    html.endTable();

    return std::make_unique<Response>(ContentTypeHtml, html.getHtmlContent());
}

Resource::ResponsePtr TopologyDebugResource::handleGet(const Request &)
{
    Topology topology;
    try {
        mSystem.getTopology(topology);
    } catch (cavs::System::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError,
                                  "Cannot get topology from fw: " + std::string(e.what()));
    }

    HtmlHelper html;

    dumpGateways(html, topology.gateways);
    dumpPipelines(html, topology.pipelines);
    dumpAllSchedulers(html, topology.schedulers);
    dumpModuleInstances(html, topology.moduleInstances);

    return std::make_unique<Response>(ContentTypeHtml, html.getHtmlContent());
}

void TopologyDebugResource::dumpGateways(HtmlHelper &html,
                                         const std::vector<dsp_fw::GatewayProps> &gateways)
{
    html.title("Gateways");
    html.paragraph("Gateway count: " + std::to_string(gateways.size()));

    static const std::vector<std::string> gatewayColumns = {"type index", "type name", "id",
                                                            "attrib"};

    html.beginTable(gatewayColumns);

    for (auto &gateway : gateways) {
        html.beginRow();

        dsp_fw::ConnectorNodeId connector(gateway.id);
        dsp_fw::ConnectorNodeId::Type type =
            static_cast<dsp_fw::ConnectorNodeId::Type>(connector.val.f.dma_type);

        html.cell(connector.val.f.dma_type);
        html.cell(dsp_fw::ConnectorNodeId::getTypeEnumHelper().toString(type));
        html.cell(connector.val.f.v_index);
        html.cell(gateway.attribs.dw);

        html.endRow();
    }

    html.endTable();
}

void TopologyDebugResource::dumpPipelines(HtmlHelper &html,
                                          const std::vector<dsp_fw::PplProps> &pipelines)
{
    /* Pipes */
    html.title("Pipelines");
    html.paragraph("Pipeline count: " + std::to_string(pipelines.size()));

    static const std::vector<std::string> columns = {
        "id",       "priority", "total_memory_bytes", "used_memory_bytes", "context_pages",
        "DP tasks", "LL tasks", "Module instances",

    };

    html.beginTable(columns);

    for (auto &pipeline : pipelines) {
        html.beginRow();

        html.cell(pipeline.id.getValue());
        html.cell(pipeline.priority);
        html.cell(pipeline.total_memory_bytes);
        html.cell(pipeline.used_memory_bytes);
        html.cell(pipeline.context_pages);
        html.cell(vectorToString(pipeline.dp_tasks));
        html.cell(vectorToString(pipeline.ll_tasks));
        html.cell(vectorToString(pipeline.module_instances));

        html.endRow();
    }

    html.endTable();
}

void TopologyDebugResource::dumpAllSchedulers(
    HtmlHelper &html, const std::vector<dsp_fw::SchedulersInfo> &allSchedulers)
{
    /* Pipes */
    html.title("Schedulers");
    html.paragraph("Core count: " + std::to_string(allSchedulers.size()));

    static const std::vector<std::string> columns = {
        "core index", "scheduler count", "schedulers",
    };

    html.beginTable(columns);

    std::size_t coreIndex = 0;
    for (auto &coreSchedulers : allSchedulers) {
        html.beginRow();

        html.cell(coreIndex);
        html.cell(coreSchedulers.scheduler_info.size());

        html.beginCell();
        dumpCoreSchedulers(html, coreSchedulers);
        html.endCell();

        html.endRow();
        coreIndex++;
    }

    html.endTable();
}

void TopologyDebugResource::dumpCoreSchedulers(HtmlHelper &html,
                                               const dsp_fw::SchedulersInfo &coreSchedulers)
{
    static const std::vector<std::string> columns = {
        "core_id", "processing_domain", "task count", "tasks",
    };

    html.beginTable(columns);

    for (auto &scheduler : coreSchedulers.scheduler_info) {
        html.beginRow();

        html.cell(scheduler.core_id);
        html.cell(scheduler.processing_domain);
        html.cell(scheduler.task_info.size());

        html.beginCell();
        dumpTasks(html, scheduler.task_info);
        html.endCell();

        html.endRow();
    }

    html.endTable();
}

void TopologyDebugResource::dumpTasks(HtmlHelper &html, const std::vector<dsp_fw::TaskProps> &tasks)
{
    static const std::vector<std::string> columns = {
        "task_id", "modules instances",
    };

    html.beginTable(columns);

    for (auto &task : tasks) {

        html.beginRow();

        html.cell(task.task_id);
        html.cell(vectorToString(task.module_instance_id));

        html.endRow();
    }

    html.endTable();
}

void TopologyDebugResource::dumpModuleInstances(
    HtmlHelper &html,
    const std::map<dsp_fw::CompoundModuleId, dsp_fw::ModuleInstanceProps> &moduleInstances)
{
    html.title("Module instances");
    html.paragraph("Module instance count: " + std::to_string(moduleInstances.size()));

    static const std::vector<std::string> columns = {
        "compound id",
        "module type id",
        "module type name",
        "instance id",
        "dp_queue_type",
        "queue_alignment",
        "cp_usage_mask",
        "stack_bytes",
        "bss_total_bytes",
        "bss_used_bytes",
        "ibs_bytes",
        "obs_bytes",
        "cpc",
        "cpc_peak",
        "input_gateway",
        "output_gateway",
        "input pins",
        "output pins",
    };

    html.beginTable(columns);

    for (auto &entry : moduleInstances) {
        html.beginRow();

        const dsp_fw::ModuleInstanceProps &module = entry.second;

        const dsp_fw::ModuleEntry &moduleEntry =
            mSystem.getModuleHandler().findModuleEntry(module.id.moduleId);

        html.cell(module.id.toString());
        html.cell(module.id.moduleId);
        html.cell(moduleEntry.getName());
        html.cell(module.id.instanceId);
        html.cell(module.dp_queue_type);
        html.cell(module.queue_alignment);
        html.cell(module.cp_usage_mask);
        html.cell(module.stack_bytes);
        html.cell(module.bss_total_bytes);
        html.cell(module.bss_used_bytes);
        html.cell(module.ibs_bytes);
        html.cell(module.obs_bytes);
        html.cell(module.cpc);
        html.cell(module.cpc_peak);
        html.cell(gatewayToString(module.input_gateway));
        html.cell(gatewayToString(module.output_gateway));

        html.beginCell();
        dumpPins(html, module.input_pins.pin_info);
        html.endCell();

        html.beginCell();
        dumpPins(html, module.output_pins.pin_info);
        html.endCell();

        html.endRow();
    }

    html.endTable();
}

void TopologyDebugResource::dumpPins(HtmlHelper &html,
                                     const std::vector<cavs::dsp_fw::PinProps> &pins)
{
    static const std::vector<std::string> columns = {
        "phys_queue_id", "stream_type", "format",
    };

    if (!pins.empty()) {
        html.beginTable(columns);

        for (auto &pin : pins) {
            html.beginRow();

            html.cell(pin.phys_queue_id);
            html.cell(dsp_fw::getStreamTypeHelper().toString(pin.stream_type));
            html.cell(pin.format.toString());

            html.endRow();
        }

        html.endTable();
    }
}

Resource::ResponsePtr ModelDumpDebugResource::handleGet(const Request &)
{
    auto guard = mInstanceModel.lock();
    auto handle = guard->get();
    if (handle == nullptr) {
        throw Response::HttpError(Response::ErrorStatus::InternalError,
                                  "Instance model is undefined.");
    }

    auto ss = std::make_unique<std::stringstream>();

    try {
        FdkToolMockGenerator generator(*ss);
        generator.setTypeModel(mTypeModel);
        generator.setSystemInstance(mSystemInstance);
        generator.setInstanceModel(*handle);
    } catch (FdkToolMockGenerator::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError,
                                  std::string("Cannot create model archive: ") + e.what());
    }

    return std::make_unique<StreamResponse>(ContentTypeZip, std::move(ss));
}

Resource::ResponsePtr AboutResource::handleGet(const Request &)
{
    HtmlHelper html;
    html.title("About");
    html.paragraph("Version: " + util::about::version());
    html.paragraph("Build configuration: " + util::about::configuration());
    return std::make_unique<Response>(ContentTypeHtml, html.getHtmlContent());
}
}
}
