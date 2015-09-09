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
#include "Tlv/TlvVoidWrapper.hpp"
#include "catch.hpp"

using namespace debug_agent::tlv;


TEST_CASE("TlvVoidWrapper", "[WrapperRead]")
{
    TlvVoidWrapper tlvVoidWrapper;

    /* Check the void wrapper accepts any size */
    CHECK(tlvVoidWrapper.isValidSize(std::numeric_limits<size_t>::max()) == true);
    CHECK(tlvVoidWrapper.isValidSize(std::numeric_limits<size_t>::min()) == true);
    CHECK(tlvVoidWrapper.isValidSize(42) == true);

    CHECK_NOTHROW(tlvVoidWrapper.readFrom(nullptr, 0));

    CHECK_NOTHROW(tlvVoidWrapper.invalidate());
}