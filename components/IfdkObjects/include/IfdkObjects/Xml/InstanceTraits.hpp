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

#pragma once

#include "IfdkObjects/Instance/Instance.hpp"
#include "IfdkObjects/Instance/Component.hpp"
#include "IfdkObjects/Instance/Subsystem.hpp"
#include "IfdkObjects/Instance/System.hpp"
#include "IfdkObjects/Instance/Service.hpp"
#include "IfdkObjects/Instance/EndPoint.hpp"
#include "IfdkObjects/Instance/Parents.hpp"
#include "IfdkObjects/Instance/InstanceRef.hpp"
#include "IfdkObjects/Instance/ComponentRef.hpp"
#include "IfdkObjects/Instance/ServiceRef.hpp"
#include "IfdkObjects/Instance/EndPointRef.hpp"
#include "IfdkObjects/Instance/SubsystemRef.hpp"
#include "IfdkObjects/Instance/SystemRef.hpp"
#include "IfdkObjects/Instance/Children.hpp"
#include "IfdkObjects/Instance/InstanceRefCollection.hpp"
#include "IfdkObjects/Instance/ComponentRefCollection.hpp"
#include "IfdkObjects/Instance/ServiceRefCollection.hpp"
#include "IfdkObjects/Instance/EndPointRefCollection.hpp"
#include "IfdkObjects/Instance/SubsystemRefCollection.hpp"
#include "IfdkObjects/Instance/Connector.hpp"
#include "IfdkObjects/Instance/Input.hpp"
#include "IfdkObjects/Instance/Inputs.hpp"
#include "IfdkObjects/Instance/Output.hpp"
#include "IfdkObjects/Instance/Outputs.hpp"
#include "IfdkObjects/Instance/Parameters.hpp"
#include "IfdkObjects/Instance/InfoParameters.hpp"
#include "IfdkObjects/Instance/ControlParameters.hpp"
#include "IfdkObjects/Instance/To.hpp"
#include "IfdkObjects/Instance/From.hpp"
#include "IfdkObjects/Instance/Link.hpp"
#include "IfdkObjects/Instance/Links.hpp"
#include "IfdkObjects/Instance/InstanceCollection.hpp"
#include "IfdkObjects/Instance/ComponentCollection.hpp"
#include "IfdkObjects/Instance/SubsystemCollection.hpp"
#include "IfdkObjects/Instance/ServiceCollection.hpp"
#include "IfdkObjects/Instance/EndPointCollection.hpp"

#include <string>

namespace debug_agent
{
namespace ifdk_objects
{
namespace xml
{

/** Traits of the "Instance" data model describing XML tags and attributes */

/** base Traits class*/
template <class T>
struct InstanceTraits
{
};

/* References */

template <>
struct InstanceTraits<instance::Ref>
{
    /* No tag because this class cannot be serializable */
    static const std::string attributeTypeName;
    static const std::string attributeInstanceId;
};

template <>
struct InstanceTraits<instance::InstanceRef>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ComponentRef>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ServiceRef>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::EndPointRef>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::SubsystemRef>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::SystemRef>
{
    static const std::string tag;
};

/* Named reference collections */

template <>
struct InstanceTraits<instance::RefCollection>
{
    /* No tag because this class cannot be serializable */
    static const std::string attributeName;
};

template <>
struct InstanceTraits<instance::InstanceRefCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ComponentRefCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ServiceRefCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::EndPointRefCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::SubsystemRefCollection>
{
    static const std::string tag;
};

/* Parents & Children */

template <>
struct InstanceTraits<instance::Parents>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Children>
{
    static const std::string tag;
};

/* Parameters */

template <>
struct InstanceTraits<instance::Parameters>
{
    /* No tag because this class cannot be serializable */
};

template <>
struct InstanceTraits<instance::InfoParameters>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ControlParameters>
{
    static const std::string tag;
};

/* Inputs / Outputs */

template <>
struct InstanceTraits<instance::Connector>
{
    /* No tag because this class cannot be serializable */
    static const std::string attributeId;
    static const std::string attributeFormat;
};

template <>
struct InstanceTraits<instance::Input>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Output>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Inputs>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Outputs>
{
    static const std::string tag;
};

/* Links*/

template <>
struct InstanceTraits<instance::To>
{
    static const std::string tag;
    static const std::string attributeTypeName;
    static const std::string attributeInstanceId;
    static const std::string attributeInputId;
};

template <>
struct InstanceTraits<instance::From>
{
    static const std::string tag;
    static const std::string attributeTypeName;
    static const std::string attributeInstanceId;
    static const std::string attributeOutputId;
};

template <>
struct InstanceTraits<instance::Link>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Links>
{
    static const std::string tag;
};

/* Main type classes */

template <>
struct InstanceTraits<instance::Instance>
{
    static const std::string tag;
    static const std::string attributeTypeName;
    static const std::string attributeInstanceId;
};

template <>
struct InstanceTraits<instance::Component>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Subsystem>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::System>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::Service>
{
    static const std::string tag;
    static const std::string attributeDirection;
};

template <>
struct InstanceTraits<instance::EndPoint>
{
    static const std::string tag;
};

/* Main instance collection */

template <>
struct InstanceTraits<instance::InstanceCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ComponentCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::SubsystemCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::ServiceCollection>
{
    static const std::string tag;
};

template <>
struct InstanceTraits<instance::EndPointCollection>
{
    static const std::string tag;
};
}
}
}
