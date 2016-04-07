/*
********************************************************************************
*                              INTEL CONFIDENTIAL
*   Copyright(C) 2015-2016 Intel Corporation. All Rights Reserved.
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
#include "Core/Resources.hpp"
#include "Core/InstanceModelConverter.hpp"
#include "Rest/CustomResponse.hpp"
#include "Rest/StreamResponse.hpp"
#include "IfdkObjects/Xml/TypeDeserializer.hpp"
#include "IfdkObjects/Xml/TypeSerializer.hpp"
#include "IfdkObjects/Xml/InstanceDeserializer.hpp"
#include "IfdkObjects/Xml/InstanceSerializer.hpp"
#include "Util/convert.hpp"

using namespace debug_agent::rest;
using namespace debug_agent::cavs;
using namespace debug_agent::ifdk_objects;

namespace debug_agent
{
namespace core
{

static const std::string ContentTypeHtml("text/html");
static const std::string ContentTypeXml("text/xml");
/**
* @fixme use the content type specified by SwAS. Currently, the SwAS does not specify
* which content type shall be used. A request has been sent to get SwAS updated. Until that,
* the rational is:
*  - the content is application specific: usage of 'application/'
*  - the content type is vendor specific: usage of standard 'vnd.'
*  - knowing the resource is an IFDK file, the client can read the streamed IFDK header to
*    know which <subsystem>:<file format> the resource is.
* @remarks http://www.iana.org/assignments/media-types/media-types.xhtml
*/
static const std::string ContentTypeIfdkFile("application/vnd.ifdk-file");

/** This method returns the value of a node part of an XML document, based on an XPath expression
 *
 *  @param const Poco::XML::Document* the XML document to parse
 *  @param const std::string& a simplified XPath expression describing the location of the node
 *  in the XML tree
 *
 *  @returns a std::string corresponding to the value of an XML node
 */
inline static const std::string getNodeValueFromXPath(const Poco::XML::Document *document,
                                                      const std::string &url)
{
    Poco::XML::Node *node = document->getNodeByPath(url);
    if (node) {
        return Poco::XML::fromXMLString(node->innerText());
    } else {
        throw Response::HttpError(Response::ErrorStatus::BadRequest,
                                  "Invalid parameters format: node for path \"" + url +
                                      "\" not found");
    }
}

Resource::ResponsePtr SystemTypeResource::handleGet(const Request &)
{
    xml::TypeSerializer serializer;
    mTypeModel.getSystem()->accept(serializer);

    return std::make_unique<Response>(ContentTypeXml, serializer.getXml());
}

Resource::ResponsePtr SystemInstanceResource::handleGet(const Request &)
{
    xml::InstanceSerializer serializer;
    mSystemInstance.accept(serializer);

    return std::make_unique<Response>(ContentTypeXml, serializer.getXml());
}

Resource::ResponsePtr TypeResource::handleGet(const Request &request)
{
    std::string typeName = request.getIdentifierValue("type_name");
    std::shared_ptr<const type::Type> typePtr = mTypeModel.getType(typeName);

    if (typePtr == nullptr) {
        throw Response::HttpError(Response::ErrorStatus::BadRequest, "Unknown type: " + typeName);
    }

    xml::TypeSerializer serializer;
    typePtr->accept(serializer);

    return std::make_unique<Response>(ContentTypeXml, serializer.getXml());
}

Resource::ResponsePtr InstanceCollectionResource::handleGet(const Request &request)
{
    xml::InstanceSerializer serializer;
    std::string typeName = request.getIdentifierValue("type_name");

    {
        const auto guard = mInstanceModel.lock();
        auto handle = guard->get();
        if (handle == nullptr) {
            throw Response::HttpError(Response::ErrorStatus::InternalError,
                                      "Instance model is undefined.");
        }
        std::shared_ptr<const instance::BaseCollection> collection =
            handle->getCollection(typeName);

        /* check nullptr using get() to avoid any KW error */
        if (collection.get() == nullptr) {
            throw Response::HttpError(Response::ErrorStatus::BadRequest,
                                      "Unknown type: " + typeName);
        }

        collection->accept(serializer);
    }

    return std::make_unique<Response>(ContentTypeXml, serializer.getXml());
}

Resource::ResponsePtr InstanceResource::handleGet(const Request &request)
{
    xml::InstanceSerializer serializer;
    std::string typeName = request.getIdentifierValue("type_name");
    std::string instanceId = request.getIdentifierValue("instance_id");

    {
        auto guard = mInstanceModel.lock();
        auto handle = guard->get();
        if (handle == nullptr) {
            throw Response::HttpError(Response::ErrorStatus::InternalError,
                                      "Instance model is undefined.");
        }
        std::shared_ptr<const instance::Instance> instancePtr =
            handle->getInstance(typeName, instanceId);

        /* check nullptr using get() to avoid any KW error */
        if (instancePtr.get() == nullptr) {
            throw Response::HttpError(Response::ErrorStatus::BadRequest,
                                      "Unknown instance: type=" + typeName + " instance_id=" +
                                          instanceId);
        }

        instancePtr->accept(serializer);
    }

    return std::make_unique<Response>(ContentTypeXml, serializer.getXml());
}

Resource::ResponsePtr RefreshSubsystemResource::handlePost(const Request &)
{
    std::shared_ptr<InstanceModel> instanceModel;
    auto guard = mInstanceModel.lock();

    try {
        InstanceModelConverter converter(mSystem);
        instanceModel = converter.createModel();
    } catch (BaseModelConverter::Exception &e) {
        /* Topology retrieving has failed: invalidate the previous one */
        guard->reset();

        throw Response::HttpError(Response::ErrorStatus::InternalError,
                                  "Cannot refresh instance model: " + std::string(e.what()));
    }

    /* Apply new topology */
    *guard.get() = instanceModel;

    return std::make_unique<Response>();
}

Resource::ResponsePtr ParameterStructureResource::handleGet(const Request &request)
{
    std::string typeName = request.getIdentifierValue("type_name");
    std::string structure;
    try {
        structure = mParamDispatcher.getParameterStructure(typeName, mKind);
    } catch (ParameterDispatcher::UnsupportedException &e) {
        throw Response::HttpError(Response::ErrorStatus::NotFound, e.what());
    } catch (ParameterDispatcher::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError, e.what());
    }

    return std::make_unique<Response>(ContentTypeXml, structure);
}

Resource::ResponsePtr ParameterValueResource::handleGet(const Request &request)
{
    std::string typeName = request.getIdentifierValue("type_name");
    std::string instanceId = request.getIdentifierValue("instance_id");

    std::string value;
    try {
        value = mParamDispatcher.getParameterValue(typeName, mKind, instanceId);
    } catch (ParameterDispatcher::UnsupportedException &e) {
        throw Response::HttpError(Response::ErrorStatus::NotFound, e.what());
    } catch (ParameterDispatcher::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError, e.what());
    }

    return std::make_unique<Response>(ContentTypeXml, value);
}

Resource::ResponsePtr ParameterValueResource::handlePut(const Request &request)
{
    /* Can set only control parameters */
    if (mKind != ParameterKind::Control) {
        throw Response::HttpError(Response::ErrorStatus::NotFound,
                                  "Can set only control parameters");
    }

    std::string typeName = request.getIdentifierValue("type_name");
    std::string instanceId = request.getIdentifierValue("instance_id");

    try {
        mParamDispatcher.setParameterValue(typeName, ParameterKind::Control, instanceId,
                                           request.getRequestContentAsString());
    } catch (ParameterDispatcher::UnsupportedException &e) {
        throw Response::HttpError(Response::ErrorStatus::NotFound, e.what());
    } catch (ParameterDispatcher::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError, e.what());
    }

    return std::make_unique<Response>();
}

/** Http response that streams from a Prober::OutputStreamResource */
class StreamResponse : public CustomResponse
{
public:
    StreamResponse(const std::string &contentType,
                   std::unique_ptr<System::OutputStreamResource> streamResource)
        : CustomResponse(contentType), mStreamResource(std::move(streamResource))
    {
    }

    void doBodyResponse(std::ostream &out) override
    {
        try {
            mStreamResource->doWriting(out);
        } catch (System::Exception &e) {
            throw Response::HttpAbort(std::string("cAVS Log stream error: ") + e.what());
        }
    }

private:
    std::unique_ptr<System::OutputStreamResource> mStreamResource;
};

Resource::ResponsePtr LogServiceStreamResource::handleGet(const Request &)
{
    /** Acquiring the log stream resource */
    auto &&resource = mSystem.tryToAcquireLogStreamResource();
    if (resource == nullptr) {
        throw Response::HttpError(Response::ErrorStatus::Locked,
                                  "Logging stream resource is already used.");
    }

    return std::make_unique<StreamResponse>(ContentTypeIfdkFile, std::move(resource));
}

ProbeId ProbeStreamResource::getProbeId(const Request &request)
{
    std::string instanceId = request.getIdentifierValue("instance_id");
    ProbeId::RawType probeIndex;
    if (!convertTo(instanceId, probeIndex)) {
        throw Response::HttpError(Response::ErrorStatus::BadRequest,
                                  "The probe index '" + instanceId + "' is invalid");
    }
    return ProbeId(probeIndex);
}

Resource::ResponsePtr ProbeStreamResource::handleGet(const Request &request)
{
    ProbeId probeId = getProbeId(request);

    auto &&resource = mSystem.tryToAcquireProbeExtractionStreamResource(probeId);
    if (resource == nullptr) {
        throw Response::HttpError(Response::ErrorStatus::Locked,
                                  "Probe extraction resource #" +
                                      std::to_string(probeId.getValue()) + " is already used.");
    }

    return std::make_unique<StreamResponse>(ContentTypeIfdkFile, std::move(resource));
}

Resource::ResponsePtr ProbeStreamResource::handlePut(const Request &request)
{
    ProbeId probeId = getProbeId(request);

    auto &&resource = mSystem.tryToAcquireProbeInjectionStreamResource(probeId);
    if (resource == nullptr) {
        throw Response::HttpError(Response::ErrorStatus::Locked,
                                  "Probe injection resource #" +
                                      std::to_string(probeId.getValue()) + " is already used.");
    }

    try {
        resource->doReading(request.getRequestStream());
    } catch (System::Exception &e) {
        throw Response::HttpError(Response::ErrorStatus::InternalError,
                                  "Probe #" + std::to_string(probeId.getValue()) + " has failed: " +
                                      std::string(e.what()));
    }

    return std::make_unique<Response>();
}
}
}
