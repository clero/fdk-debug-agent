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

#include "Core/TypeModel.hpp"
#include "Core/InstanceModel.hpp"
#include "Rest/Resource.hpp"
#include "cAVS/System.hpp"
#include "Util/ExclusiveResource.hpp"
#include "ParameterMgrPlatformConnector.h"
#include "ElementHandle.h"

namespace debug_agent
{
namespace core
{

using ExclusiveInstanceModel = util::ExclusiveResource<std::shared_ptr<InstanceModel>>;

class SystemResource : public rest::Resource
{
public:
    explicit SystemResource(cavs::System &system) : mSystem(system) {}
protected:
    cavs::System &mSystem;
};

/** This resource returns the System Type, containing Subsystems Types (XML) */
class SystemTypeResource : public rest::Resource
{
public:
    SystemTypeResource(TypeModel &model) : mTypeModel(model) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
private:
    TypeModel &mTypeModel;
};

/** This resource returns the System instance, containing Subsystem instances (XML) */
class SystemInstanceResource : public rest::Resource
{
public:
    SystemInstanceResource(ExclusiveInstanceModel &instanceModel) :
        mInstanceModel(instanceModel) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
private:
    ExclusiveInstanceModel &mInstanceModel;
};

/** This resource returns a subsystem type (XML) */
class TypeResource : public rest::Resource
{
public:
    TypeResource(TypeModel &model) : mTypeModel(model) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
private:
    TypeModel &mTypeModel;
};

class InstanceResource : public rest::Resource
{
public:
    InstanceResource(ExclusiveInstanceModel &instanceModel) : mInstanceModel(instanceModel) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
private:
    ExclusiveInstanceModel &mInstanceModel;
};

class InstanceCollectionResource : public rest::Resource
{
public:
    InstanceCollectionResource(ExclusiveInstanceModel &instanceModel) :
        mInstanceModel(instanceModel) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
private:
    ExclusiveInstanceModel &mInstanceModel;
};

class RefreshSubsystemResource : public SystemResource
{
public:
    RefreshSubsystemResource(cavs::System &system, ExclusiveInstanceModel &instanceModel) :
        SystemResource(system), mInstanceModel(instanceModel) {}
protected:
    virtual void handlePost(const rest::Request &request, rest::Response &response) override;
private:
    ExclusiveInstanceModel &mInstanceModel;
};


/** This resource returns control parameters of a module instance of a Subsystem (XML) */
class ControlParametersModuleInstanceResource : public SystemResource
{
public:
    ControlParametersModuleInstanceResource(cavs::System &system,
        CParameterMgrPlatformConnector &parameterMgrPlatformConnector,
        const std::string &moduleName,
        const uint16_t moduleId);
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
    virtual void handlePut(const rest::Request &request, rest::Response &response) override;

private:
    CElementHandle* getModuleControlElement() const;
    CElementHandle *getChildElementHandle(
        const CElementHandle *moduleElementHandle, uint32_t childId) const;
    uint32_t getElementMapping(const CElementHandle *elementHandle) const;
    CParameterMgrPlatformConnector &mParameterMgrPlatformConnector;
    const std::string mModuleName;
    const uint16_t mModuleId;
};

/** This resource returns the Log Parameters for an Instance of a Subsystem (XML) */
class SubsystemInstanceLogParametersResource : public SystemResource
{
public:
    SubsystemInstanceLogParametersResource(cavs::System &system) : SystemResource(system) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
    virtual void handlePut(const rest::Request &request, rest::Response &response) override;
};

/** This resource returns the Log Control Parameters for an Instance of a Subsystem (XML) */
class SubsystemInstanceLogControlParametersResource : public SystemResource
{
public:
    SubsystemInstanceLogControlParametersResource(cavs::System &system) : SystemResource(system) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
    virtual void handlePut(const rest::Request &request, rest::Response &response) override;
};

/** This resource returns the Log Parameters for a Subsystem Type (XML) */
class SubsystemTypeLogParametersResource : public SystemResource
{
public:
    SubsystemTypeLogParametersResource(cavs::System &system) : SystemResource(system) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
};

/** This resource returns the Log Stream for a Subsystem Instance (XML) */
class SubsystemInstanceLogStreamResource : public SystemResource
{
public:
    SubsystemInstanceLogStreamResource(cavs::System &system) : SystemResource(system) {}
protected:
    virtual void handleGet(const rest::Request &request, rest::Response &response) override;
};

}
}


