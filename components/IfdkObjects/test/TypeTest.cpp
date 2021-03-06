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
#include "IfdkObjects/Xml/TypeSerializer.hpp"
#include "IfdkObjects/Xml/TypeDeserializer.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace debug_agent::ifdk_objects::type;
using namespace debug_agent::ifdk_objects::xml;

void populateDescription(Description &desc)
{
    desc.setValue("my desc");
}

void populateCharacteristics(Characteristics &characteristics)
{
    characteristics.add(Characteristic("c1", "v1"));
    characteristics.add(Characteristic("c2", "v2"));
    characteristics.add(Characteristic("c3", "v3"));
}

void populateChildren(Children &children)
{
    std::shared_ptr<TypeRefCollection> typeColl = std::make_shared<TypeRefCollection>("types");
    typeColl->add(TypeRef("type1"));

    std::shared_ptr<ComponentRefCollection> compColl =
        std::make_shared<ComponentRefCollection>("components");
    compColl->add(ComponentRef("comp1"));

    std::shared_ptr<ServiceRefCollection> servColl =
        std::make_shared<ServiceRefCollection>("services");
    servColl->add(ServiceRef("service1"));

    children.add(typeColl);
    children.add(compColl);
    children.add(servColl);
}

void populateInputs(Inputs &inputs)
{
    inputs.add(Input("id1", "name1"));
    inputs.add(Input("id2", "name2"));
}

void populateOutputs(Outputs &outputs)
{
    outputs.add(Output("id1", "name1"));
    outputs.add(Output("id2", "name2"));
    outputs.add(Output("id3", "name3"));
}

void populateCategories(Categories &categories)
{
    categories.add(std::make_shared<TypeRef>("type1"));
    categories.add(std::make_shared<ComponentRef>("comp1"));
    categories.add(std::make_shared<ServiceRef>("serv1"));
}

template <typename T>
void testTypeRef(const std::string &expectedXml)
{
    /* Serialize */
    T typeRef("my_ref");

    TypeSerializer serializer;
    typeRef.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == expectedXml);

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    T deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == typeRef);
}

TEST_CASE("Type serializer: TypeRef")
{
    testTypeRef<TypeRef>("<type Name=\"my_ref\"/>\n");
}

TEST_CASE("Type serializer: ComponentRef")
{
    testTypeRef<ComponentRef>("<component_type Name=\"my_ref\"/>\n");
}

TEST_CASE("Type serializer: ServiceRef")
{
    testTypeRef<ServiceRef>("<service_type Name=\"my_ref\"/>\n");
}

TEST_CASE("Type serializer: EndPointRef")
{
    testTypeRef<EndPointRef>("<endpoint_type Name=\"my_ref\"/>\n");
}

TEST_CASE("Type serializer: SubsystemRef")
{
    testTypeRef<SubsystemRef>("<subsystem_type Name=\"my_ref\"/>\n");
}

TEST_CASE("Type serializer: Categories")
{
    /* Serialize */
    Categories categories;
    categories.add(std::make_shared<TypeRef>("type2"));
    categories.add(std::make_shared<ComponentRef>("comp2"));
    categories.add(std::make_shared<ServiceRef>("serv2"));
    categories.add(std::make_shared<EndPointRef>("ep2"));

    TypeSerializer serializer;
    categories.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<categories>\n"
                 "    <type Name=\"type2\"/>\n"
                 "    <component_type Name=\"comp2\"/>\n"
                 "    <service_type Name=\"serv2\"/>\n"
                 "    <endpoint_type Name=\"ep2\"/>\n"
                 "</categories>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Categories deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == categories);
}

TEST_CASE("Type serializer: type ref collection")
{
    /* Serialize */
    TypeRefCollection typeRef("my_type_ref_coll");
    typeRef.add(TypeRef("type1"));
    typeRef.add(TypeRef("type2"));

    TypeSerializer serializer;
    typeRef.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<collection Name=\"my_type_ref_coll\">\n"
                 "    <type Name=\"type1\"/>\n"
                 "    <type Name=\"type2\"/>\n"
                 "</collection>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    TypeRefCollection deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == typeRef);
}

TEST_CASE("Type serializer: compo ref collection")
{
    /* Serialize */
    ComponentRefCollection compoRef("my_comp_ref_coll");
    compoRef.add(ComponentRef("compo1"));
    compoRef.add(ComponentRef("compo2"));

    TypeSerializer serializer;
    compoRef.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<component_collection Name=\"my_comp_ref_coll\">\n"
                 "    <component_type Name=\"compo1\"/>\n"
                 "    <component_type Name=\"compo2\"/>\n"
                 "</component_collection>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    ComponentRefCollection deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == compoRef);
}

TEST_CASE("Type serializer: service ref collection")
{
    /* Serialize */
    ServiceRefCollection serviceRef("my_service_ref_coll");
    serviceRef.add(ServiceRef("service1"));
    serviceRef.add(ServiceRef("service2"));

    TypeSerializer serializer;
    serviceRef.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<service_collection Name=\"my_service_ref_coll\">\n"
                 "    <service_type Name=\"service1\"/>\n"
                 "    <service_type Name=\"service2\"/>\n"
                 "</service_collection>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    ServiceRefCollection deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == serviceRef);
}

TEST_CASE("Type serializer: endpoint ref collection")
{
    /* Serialize */
    EndPointRefCollection endPointRef("my_endPoint_ref_coll");
    endPointRef.add(EndPointRef("endPoint1"));
    endPointRef.add(EndPointRef("endPoint2"));

    TypeSerializer serializer;
    endPointRef.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<endpoint_collection Name=\"my_endPoint_ref_coll\">\n"
                 "    <endpoint_type Name=\"endPoint1\"/>\n"
                 "    <endpoint_type Name=\"endPoint2\"/>\n"
                 "</endpoint_collection>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    EndPointRefCollection deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == endPointRef);
}

TEST_CASE("Type serializer: subsystem ref collection")
{
    /* Serialize */
    SubsystemRefCollection subsystemRef("my_subsystem_ref_coll");
    subsystemRef.add(SubsystemRef("subsystem1"));
    subsystemRef.add(SubsystemRef("subsystem2"));

    TypeSerializer serializer;
    subsystemRef.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<subsystem_collection Name=\"my_subsystem_ref_coll\">\n"
                 "    <subsystem_type Name=\"subsystem1\"/>\n"
                 "    <subsystem_type Name=\"subsystem2\"/>\n"
                 "</subsystem_collection>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    SubsystemRefCollection deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == subsystemRef);
}

TEST_CASE("Type serializer: Description")
{
    /* Serialize */
    Description desc("my desc");

    TypeSerializer serializer;
    desc.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<description>my desc</description>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Description deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == desc);
}

TEST_CASE("Type serializer: Characteristic")
{
    /* Serialize */
    Characteristic characteristic("name", "value");

    TypeSerializer serializer;
    characteristic.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<characteristic Name=\"name\">value</characteristic>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Characteristic deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == characteristic);
}

TEST_CASE("Type serializer: Characteristics")
{
    /* Serialize */
    Characteristics characteristics;
    populateCharacteristics(characteristics);

    TypeSerializer serializer;
    characteristics.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<characteristics>\n"
                 "    <characteristic Name=\"c1\">v1</characteristic>\n"
                 "    <characteristic Name=\"c2\">v2</characteristic>\n"
                 "    <characteristic Name=\"c3\">v3</characteristic>\n"
                 "</characteristics>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Characteristics deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == characteristics);
}

TEST_CASE("Type serializer: Info parameters")
{
    /* Serialize */
    InfoParameters params;

    TypeSerializer serializer;
    params.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<info_parameters/>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    InfoParameters deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == params);
}

TEST_CASE("Type serializer: Control parameters")
{
    /* Serialize */
    ControlParameters params;

    TypeSerializer serializer;
    params.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<control_parameters/>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    ControlParameters deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == params);
}

TEST_CASE("Type serializer: children")
{
    /* Serialize */
    Children children;
    populateChildren(children);

    TypeSerializer serializer;
    children.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<children>\n"
                 "    <collection Name=\"types\">\n"
                 "        <type Name=\"type1\"/>\n"
                 "    </collection>\n"
                 "    <component_collection Name=\"components\">\n"
                 "        <component_type Name=\"comp1\"/>\n"
                 "    </component_collection>\n"
                 "    <service_collection Name=\"services\">\n"
                 "        <service_type Name=\"service1\"/>\n"
                 "    </service_collection>\n"
                 "</children>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Children deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == children);
}

TEST_CASE("Type serializer: Type")
{
    /* Serialize */
    Type type("my_type");
    populateDescription(type.getDescription());
    populateCharacteristics(type.getCharacteristics());
    populateChildren(type.getChildren());

    TypeSerializer serializer;
    type.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<type Name=\"my_type\">\n"
                 "    <description>my desc</description>\n"
                 "    <characteristics>\n"
                 "        <characteristic Name=\"c1\">v1</characteristic>\n"
                 "        <characteristic Name=\"c2\">v2</characteristic>\n"
                 "        <characteristic Name=\"c3\">v3</characteristic>\n"
                 "    </characteristics>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children>\n"
                 "        <collection Name=\"types\">\n"
                 "            <type Name=\"type1\"/>\n"
                 "        </collection>\n"
                 "        <component_collection Name=\"components\">\n"
                 "            <component_type Name=\"comp1\"/>\n"
                 "        </component_collection>\n"
                 "        <service_collection Name=\"services\">\n"
                 "            <service_type Name=\"service1\"/>\n"
                 "        </service_collection>\n"
                 "    </children>\n"
                 "</type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Type deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == type);
}

TEST_CASE("Type serializer: Service")
{
    /* Serialize */
    Service service("my_service");
    populateDescription(service.getDescription());
    populateCharacteristics(service.getCharacteristics());
    populateChildren(service.getChildren());

    TypeSerializer serializer;
    service.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<service_type Name=\"my_service\">\n"
                 "    <description>my desc</description>\n"
                 "    <characteristics>\n"
                 "        <characteristic Name=\"c1\">v1</characteristic>\n"
                 "        <characteristic Name=\"c2\">v2</characteristic>\n"
                 "        <characteristic Name=\"c3\">v3</characteristic>\n"
                 "    </characteristics>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children>\n"
                 "        <collection Name=\"types\">\n"
                 "            <type Name=\"type1\"/>\n"
                 "        </collection>\n"
                 "        <component_collection Name=\"components\">\n"
                 "            <component_type Name=\"comp1\"/>\n"
                 "        </component_collection>\n"
                 "        <service_collection Name=\"services\">\n"
                 "            <service_type Name=\"service1\"/>\n"
                 "        </service_collection>\n"
                 "    </children>\n"
                 "</service_type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Service deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == service);
}

TEST_CASE("Type serializer: EndPoint")
{
    /* Serialize */
    EndPoint endPoint("my_endPoint", EndPoint::Direction::Bidirectional);

    TypeSerializer serializer;
    endPoint.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<endpoint_type Direction=\"Bidirectional\" Name=\"my_endPoint\">\n"
                 "    <description/>\n"
                 "    <characteristics/>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children/>\n"
                 "</endpoint_type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    EndPoint deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == endPoint);
}

TEST_CASE("Type serializer: Input")
{
    /* Serialize */
    Input connector("my_id", "my_name");

    TypeSerializer serializer;
    connector.accept(serializer);

    std::string xml = serializer.getXml();

    CHECK(xml == "<input Id=\"my_id\" Name=\"my_name\"/>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Input deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == connector);
}

TEST_CASE("Type serializer: Output")
{
    /* Serialize */
    Output connector("my_id", "my_name");

    TypeSerializer serializer;
    connector.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<output Id=\"my_id\" Name=\"my_name\"/>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Output deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == connector);
}

TEST_CASE("Type serializer: Inputs")
{
    /* Serialize */
    Inputs connectors;
    populateInputs(connectors);

    TypeSerializer serializer;
    connectors.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<inputs>\n"
                 "    <input Id=\"id1\" Name=\"name1\"/>\n"
                 "    <input Id=\"id2\" Name=\"name2\"/>\n"
                 "</inputs>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Inputs deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == connectors);
}

TEST_CASE("Type serializer: Outputs")
{
    /* Serialize */
    Outputs connectors;
    populateOutputs(connectors);

    TypeSerializer serializer;
    connectors.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<outputs>\n"
                 "    <output Id=\"id1\" Name=\"name1\"/>\n"
                 "    <output Id=\"id2\" Name=\"name2\"/>\n"
                 "    <output Id=\"id3\" Name=\"name3\"/>\n"
                 "</outputs>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Outputs deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == connectors);
}

TEST_CASE("Type serializer: Component")
{
    /* Serialize */
    Component compo("my_compo");
    populateDescription(compo.getDescription());
    populateCharacteristics(compo.getCharacteristics());
    populateChildren(compo.getChildren());
    populateInputs(compo.getInputs());
    populateOutputs(compo.getOutputs());

    TypeSerializer serializer;
    compo.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<component_type Name=\"my_compo\">\n"
                 "    <description>my desc</description>\n"
                 "    <characteristics>\n"
                 "        <characteristic Name=\"c1\">v1</characteristic>\n"
                 "        <characteristic Name=\"c2\">v2</characteristic>\n"
                 "        <characteristic Name=\"c3\">v3</characteristic>\n"
                 "    </characteristics>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children>\n"
                 "        <collection Name=\"types\">\n"
                 "            <type Name=\"type1\"/>\n"
                 "        </collection>\n"
                 "        <component_collection Name=\"components\">\n"
                 "            <component_type Name=\"comp1\"/>\n"
                 "        </component_collection>\n"
                 "        <service_collection Name=\"services\">\n"
                 "            <service_type Name=\"service1\"/>\n"
                 "        </service_collection>\n"
                 "    </children>\n"
                 "    <inputs>\n"
                 "        <input Id=\"id1\" Name=\"name1\"/>\n"
                 "        <input Id=\"id2\" Name=\"name2\"/>\n"
                 "    </inputs>\n"
                 "    <outputs>\n"
                 "        <output Id=\"id1\" Name=\"name1\"/>\n"
                 "        <output Id=\"id2\" Name=\"name2\"/>\n"
                 "        <output Id=\"id3\" Name=\"name3\"/>\n"
                 "    </outputs>\n"
                 "</component_type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Component deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == compo);
}

TEST_CASE("Type serializer: Subsystem")
{
    /* Serialize */
    Subsystem subsystem("my_subsystem");
    populateDescription(subsystem.getDescription());
    populateCharacteristics(subsystem.getCharacteristics());
    populateChildren(subsystem.getChildren());
    populateCategories(subsystem.getCategories());
    populateInputs(subsystem.getInputs());
    populateOutputs(subsystem.getOutputs());

    TypeSerializer serializer;
    subsystem.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<subsystem_type Name=\"my_subsystem\">\n"
                 "    <description>my desc</description>\n"
                 "    <characteristics>\n"
                 "        <characteristic Name=\"c1\">v1</characteristic>\n"
                 "        <characteristic Name=\"c2\">v2</characteristic>\n"
                 "        <characteristic Name=\"c3\">v3</characteristic>\n"
                 "    </characteristics>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children>\n"
                 "        <collection Name=\"types\">\n"
                 "            <type Name=\"type1\"/>\n"
                 "        </collection>\n"
                 "        <component_collection Name=\"components\">\n"
                 "            <component_type Name=\"comp1\"/>\n"
                 "        </component_collection>\n"
                 "        <service_collection Name=\"services\">\n"
                 "            <service_type Name=\"service1\"/>\n"
                 "        </service_collection>\n"
                 "    </children>\n"
                 "    <inputs>\n"
                 "        <input Id=\"id1\" Name=\"name1\"/>\n"
                 "        <input Id=\"id2\" Name=\"name2\"/>\n"
                 "    </inputs>\n"
                 "    <outputs>\n"
                 "        <output Id=\"id1\" Name=\"name1\"/>\n"
                 "        <output Id=\"id2\" Name=\"name2\"/>\n"
                 "        <output Id=\"id3\" Name=\"name3\"/>\n"
                 "    </outputs>\n"
                 "    <categories>\n"
                 "        <type Name=\"type1\"/>\n"
                 "        <component_type Name=\"comp1\"/>\n"
                 "        <service_type Name=\"serv1\"/>\n"
                 "    </categories>\n"
                 "</subsystem_type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    Subsystem deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == subsystem);
}

TEST_CASE("Type serializer: System")
{
    /* Serialize */
    System system("my_system");
    populateDescription(system.getDescription());
    populateCharacteristics(system.getCharacteristics());

    std::shared_ptr<SubsystemRefCollection> coll =
        std::make_shared<SubsystemRefCollection>("subsystems");
    coll->add(SubsystemRef("subsystem1"));
    coll->add(SubsystemRef("subsystem2"));
    system.getChildren().add(coll);

    TypeSerializer serializer;
    system.accept(serializer);

    std::string xml = serializer.getXml();
    CHECK(xml == "<system_type Name=\"my_system\">\n"
                 "    <description>my desc</description>\n"
                 "    <characteristics>\n"
                 "        <characteristic Name=\"c1\">v1</characteristic>\n"
                 "        <characteristic Name=\"c2\">v2</characteristic>\n"
                 "        <characteristic Name=\"c3\">v3</characteristic>\n"
                 "    </characteristics>\n"
                 "    <info_parameters/>\n"
                 "    <control_parameters/>\n"
                 "    <children>\n"
                 "        <subsystem_collection Name=\"subsystems\">\n"
                 "            <subsystem_type Name=\"subsystem1\"/>\n"
                 "            <subsystem_type Name=\"subsystem2\"/>\n"
                 "        </subsystem_collection>\n"
                 "    </children>\n"
                 "    <inputs/>\n"
                 "    <outputs/>\n"
                 "</system_type>\n");

    /* Deserialize */
    TypeDeserializer deserializer(xml);
    System deserializedInstance;

    CHECK_NOTHROW(deserializedInstance.accept(deserializer));
    CHECK(deserializedInstance == system);
}
