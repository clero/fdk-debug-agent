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
#include "Tlv/TlvWrapper.hpp"
#include "Util/Buffer.hpp"
#include "catch.hpp"

using namespace debug_agent::tlv;
using namespace debug_agent::util;

struct ATestValueType
{
    int anIntField;
    char aCharField;
    short aShortField;

    bool operator==(const ATestValueType &other)
    {
        return anIntField == other.anIntField && aCharField == other.aCharField &&
               aShortField == other.aShortField;
    }

    void fromStream(ByteStreamReader &reader)
    {
        reader.read(anIntField);
        reader.read(aCharField);
        reader.read(aShortField);
    }
};

TEST_CASE("TlvWrapper", "[WrapperRead]")
{
    ATestValueType testValue{};
    bool testValueIsValid = false;
    TlvWrapper<ATestValueType> tlvWrapper(testValue, testValueIsValid);

    CHECK(testValueIsValid == false);

    Buffer buffer{210, 4, 0, 0, 56, 21, 3};

    ATestValueType valueToBeRead = {1234, 56, 789};

    tlvWrapper.readFrom(buffer);
    CHECK(testValue == valueToBeRead);
    CHECK(testValueIsValid == true);

    tlvWrapper.invalidate();
    CHECK(testValueIsValid == false);
}