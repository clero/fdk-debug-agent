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

#include "Util/EnumHelper.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <memory>

/* Forward declaration of internal types */
class CParameterMgrPlatformConnector;
class CElementHandle;

namespace debug_agent
{
namespace parameterSerializer
{
/**
 * Parameter Serializer
 */
class ParameterSerializer
{
public:
    class Exception final : public std::logic_error
    {
    public:
        explicit Exception(const std::string &what)
        : std::logic_error(what)
        {}
    };

    /**
     * Constructor
     * @param[in] configurationFilePath path to the XML configuration file of the pfw instance
     */
    ParameterSerializer(const std::string configurationFilePath);

    /**
     * Destructor implementation shall not be inlined because of the use of unique_ptr on
     * parameter-framework plateform conector.
     */
    virtual ~ParameterSerializer();

    /**
     * Parameter kind enumerate
     */
    enum class ParameterKind
    {
        Info,
        Control
    };

    /** enum helper (string conversion, type validation) for ParameterKind enum  */
    static const util::EnumHelper<ParameterKind> &parameterKindHelper()
    {
        static util::EnumHelper<ParameterKind> helper({
            { ParameterKind::Info, "info" },
            { ParameterKind::Control, "control" },
        });
        return helper;
    }

    /**
     * This method returns the children of the info/control subnode an element.
     * @param[in] subsystemName is the name of the subsystem (eg. cavs)
     * @param[in] elementName is the name of the element
     * @param[in] parameterKind is the kind of subnode (eg. control or info)
     * @return a map containing the childId and its corresponding name of children of the element
     * @throw ParameterSerializer::Exception
     */
    std::map<uint32_t, std::string> getChildren(
        const std::string &subsystemName,
        const std::string &elementName,
        ParameterKind parameterKind) const;

    /**
     * This method serializes an XML string containing the settings of one parameter (child)
     * of an element in a binary format.
     * @param[in] subsystemName is the name of the subsystem (eg. cavs)
     * @param[in] elementName is the name of the element
     * @param[in] parameterKind is the kind of subnode (eg. control or info)
     * @param[in] parameterName is the name of the parameter to serialize
     * @param[in] parameterAsXml is a string contaning the parameter settings in XML format
     * @return a binary payload containing the parameter settings
     * @throw ParameterSerializer::Exception
     */
    std::vector<uint8_t> xmlToBinary(
        const std::string &subsystemName,
        const std::string &elementName,
        ParameterKind parameterKind,
        const std::string &parameterName,
        const std::string parameterAsXml) const;

    /**
     * This method serializes binary payload containing the settings of one parameter (child)
     * of an element in an XML string.
     * @param[in] subsystemName is the name of the subsystem (eg. cavs)
     * @param[in] elementName is the name of the element
     * @param[in] parameterKind is the kind of subnode (eg. control or info)
     * @param[in] parameterName is the name of the parameter to serialize
     * @param[in] parameterPayload a binary payload containing the parameter settings
     * @return a string contaning the parameter settings in XML format
     * @throw ParameterSerializer::Exception
     */
    std::string binaryToXml(
        const std::string &subsystemName,
        const std::string &elementName,
        ParameterKind parameterKind,
        const std::string &parameterName,
        const std::vector<uint8_t> &parameterPayload) const;

private:
    std::unique_ptr<CElementHandle> getElement(
        const std::string &subsystemName,
        const std::string &elementName,
        ParameterKind parameterKind) const;

    std::unique_ptr<CElementHandle> getChildElementHandle(
        const std::string &subsystemName,
        const std::string &elementName,
        ParameterKind parameterKind,
        const std::string &parameterName) const;

    const std::unique_ptr<CParameterMgrPlatformConnector> mParameterMgrPlatformConnector;

};

}
}