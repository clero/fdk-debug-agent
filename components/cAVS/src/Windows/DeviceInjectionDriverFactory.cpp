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

#include "cAVS/Windows/DeviceInjectionDriverFactory.hpp"
#include "cAVS/Windows/Driver.hpp"

namespace debug_agent
{
namespace cavs
{
namespace windows
{

std::unique_ptr<cavs::Driver> DeviceInjectionDriverFactory::newDriver() const
{
    if (mInjectedDevice == nullptr) {
        /* mInjectedDevice is null -> the cause is that newDriver() has already been called,
         * leading to transfert mInjectedDevice unique pointer content, setting it to null */
        throw Exception("The injected device has already been used. "
            "Please call newDriver() once.");
    }

    /* After this call mInjectedDevice will be null */
    return std::unique_ptr<cavs::Driver>(new windows::Driver(std::move(mInjectedDevice)));
}

}
}
}