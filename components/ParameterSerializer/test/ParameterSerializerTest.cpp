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

#include <ParameterSerializer/ParameterSerializer.hpp>
#include <Util/StringHelper.hpp>
#include <Util/Buffer.hpp>
#include <TestCommon/TestHelpers.hpp>
#include <ostream>
#include <string>
#include <stdexcept>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace debug_agent::parameter_serializer;
using namespace debug_agent::util;

// Dirty: using FunctionTests data files
// @todo: fix it.
static const std::string rootDataPath = PROJECT_PATH "../../FunctionalTests/data/FunctionalTests/";

static const std::string pfwConfFilePath =
    rootDataPath + "pfw/ParameterFrameworkConfigurationDBGA.xml";

static const Buffer aecControlParameterPayload = {
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
static const Buffer nsControlParameterPayload = {
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1, 0xFF, 0xF1,
    0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const std::string aecUuid("00000001-0001-0000-0100-000001000000");

/** @return the file content as string */
std::string fileContent(const std::string &name)
{
    std::string fileName = rootDataPath + "http/" + name;

    std::ifstream file(fileName);
    if (!file) { /* using operator () to check stream health */
        throw std::logic_error("Unknown file: " + fileName);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (file.bad()) {
        throw std::logic_error("Error while reading file: " + fileName);
    }

    return StringHelper::trim(content) + "\n"; /* Poco xml library puts a '\n' on the last line. */
}

std::string xmlFile(const std::string &name)
{
    return fileContent(name + ".xml");
}

std::string htmlFile(const std::string &name)
{
    return fileContent(name + ".html");
}
TEST_CASE("Test parameter serializer instanciation fails")
{
    ParameterSerializer parameterSerializer("badFilePath");

    std::map<uint32_t, std::string> children;
    CHECK_THROWS_AS_MSG(children = parameterSerializer.getChildren(
                            "foo", "bar", ParameterSerializer::ParameterKind::Control),
                        ParameterSerializer::Exception, "Platform connector not available");
}

TEST_CASE("Test parameter serializer instanciation succeed")
{
    try {
        ParameterSerializer parameterSerializer(pfwConfFilePath);
    } catch (ParameterSerializer::Exception &e) {
        /* Mocked device destructor throws an exception when the test vector is not consumed */

        INFO("ParameterSerializer::Exception should not have been thrown.");
        INFO(std::string(e.what()));
        CHECK(false);
        return;
    }
    CHECK(true);
}

TEST_CASE("Test parameter serializer getChildren()")
{
    std::map<uint32_t, std::string> children;

    ParameterSerializer parameterSerializer(pfwConfFilePath);

    // Check bad subsystem name behavior
    CHECK_THROWS_AS_MSG(children = parameterSerializer.getChildren(
                            "badSubsystem", aecUuid, ParameterSerializer::ParameterKind::Control),
                        ParameterSerializer::ElementNotFound,
                        "/BXTN/badSubsystem/categories/" + aecUuid + "/control/");

    // Check bad module name behavior
    CHECK_THROWS_AS_MSG(children = parameterSerializer.getChildren(
                            "cavs", "badModule", ParameterSerializer::ParameterKind::Control),
                        ParameterSerializer::ElementNotFound,
                        "/BXTN/cavs/categories/badModule/control/");

    CHECK_NOTHROW(children = parameterSerializer.getChildren(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control));

    // module "aec" is compound of 1 "aec parameter" and one "ns parameter".
    // Check module aec has 2 parameters
    size_t childrenCount = children.size();
    CHECK(childrenCount == 2);
}

TEST_CASE("Test parameter serializer getMapping()")
{
    ParameterSerializer parameterSerializer(pfwConfFilePath);

    std::map<uint32_t, std::string> children = parameterSerializer.getChildren(
        "cavs", aecUuid, ParameterSerializer::ParameterKind::Control);

    std::string mapping;
    // Check bad mapping key behavior
    CHECK_THROWS_AS_MSG(mapping = parameterSerializer.getMapping(
                            "cavs", aecUuid, ParameterSerializer::ParameterKind::Control,
                            children[1], "badMappingKey"),
                        ParameterSerializer::Exception, "Mapping \"badMappingKey\" not found for "
                                                        "/BXTN/cavs/categories/" +
                                                            aecUuid + "/control/NoiseReduction");

    // Check bad subsystem name behavior
    CHECK_NOTHROW(
        mapping = parameterSerializer.getMapping(
            "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, children[1], "ParamId"));

    CHECK(mapping == "25");

    // module "aec" is compound of 1 "aec parameter" and one "ns parameter".
    // Check module aec has 2 parameters
    size_t childrenCount = children.size();
    CHECK(childrenCount == 2);
}

TEST_CASE("Test parameter serializer binary to xml")
{
    ParameterSerializer parameterSerializer(pfwConfFilePath);

    std::map<uint32_t, std::string> children = parameterSerializer.getChildren(
        "cavs", aecUuid, ParameterSerializer::ParameterKind::Control);

    std::string aecControlParameter;
    // Check bad payload behavior
    const Buffer badPayload = {0xDD};
    CHECK_THROWS_AS_MSG(
        aecControlParameter = parameterSerializer.binaryToXml(
            "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, children[0], badPayload),
        ParameterSerializer::Exception,
        "Not able to set payload for AcousticEchoCanceler : Wrong size: Expected: 642 "
        "Provided: 1");

    // Check bad parameter name behavior
    CHECK_THROWS_AS_MSG(aecControlParameter = parameterSerializer.binaryToXml(
                            "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, "badName",
                            aecControlParameterPayload),
                        ParameterSerializer::Exception,
                        "Child badName not found for /BXTN/cavs/categories/" + aecUuid +
                            "/control");

    // Check child 0 which is aec parameter
    CHECK_NOTHROW(aecControlParameter = parameterSerializer.binaryToXml(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, children[0],
                      aecControlParameterPayload));

    CHECK(aecControlParameter == xmlFile("instance_aec_control_params"));

    // Check child 1 which is ns parameter
    std::string nsControlParameter;
    CHECK_NOTHROW(nsControlParameter = parameterSerializer.binaryToXml(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, children[1],
                      nsControlParameterPayload));

    CHECK(nsControlParameter == xmlFile("instance_ns_control_params"));
}

TEST_CASE("Test parameter serializer xml to binary")
{
    ParameterSerializer parameterSerializer(pfwConfFilePath);

    std::map<uint32_t, std::string> children = parameterSerializer.getChildren(
        "cavs", aecUuid, ParameterSerializer::ParameterKind::Control);

    Buffer localAecControlParameterPayload;
    // Check case when XML content is bad.
    CHECK_THROWS_AS_MSG(
        localAecControlParameterPayload = parameterSerializer.xmlToBinary(
            "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, "AcousticEchoCanceler",
            "<badXmlContent>"),
        ParameterSerializer::Exception,
        "Not able to set XML stream for /BXTN/cavs/categories/" + aecUuid +
            "/control/AcousticEchoCanceler : "
            ":1:16: Premature end of data in tag badXmlContent line 1\n\nlibxml failed to read");

    // Check child 0 which is aec parameter
    CHECK_NOTHROW(localAecControlParameterPayload = parameterSerializer.xmlToBinary(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control,
                      "AcousticEchoCanceler", xmlFile("instance_aec_control_params")));

    CHECK(localAecControlParameterPayload == aecControlParameterPayload);

    // Check child 1 which is ns parameter
    Buffer localNsControlParameterPayload;
    CHECK_NOTHROW(localNsControlParameterPayload = parameterSerializer.xmlToBinary(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control,
                      "NoiseReduction", xmlFile("instance_ns_control_params")));

    CHECK(localNsControlParameterPayload == nsControlParameterPayload);
}

TEST_CASE("Test parameter serializer get structure XML")
{
    ParameterSerializer parameterSerializer(pfwConfFilePath);

    std::map<uint32_t, std::string> children = parameterSerializer.getChildren(
        "cavs", aecUuid, ParameterSerializer::ParameterKind::Control);

    Buffer localAecControlParameterPayload;

    std::string aecControlParameter;
    CHECK_NOTHROW(aecControlParameter = parameterSerializer.getStructureXml(
                      "cavs", aecUuid, ParameterSerializer::ParameterKind::Control, children[0]));

    CHECK(aecControlParameter == xmlFile("parameter_aec_type_control_params"));
}
