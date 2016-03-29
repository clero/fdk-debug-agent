/*
 ********************************************************************************
 *                              INTEL CONFIDENTIAL
 *   Copyright(C) 2016 Intel Corporation. All Rights Reserved.
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

#include "Util/Buffer.hpp"
#include <exception>
#include <string>
#include <inttypes.h>
#include <stdexcept>

namespace debug_agent
{
namespace cavs
{
namespace linux
{

/** This class abstracts a Linux Debug file system (open,close,read,write...)
 */
class Device
{
public:
    struct Exception : std::logic_error
    {
        using std::logic_error::logic_error;
    };

    /** @throw Device::Exception if the device initialization has failed */
    Device() = default;
    virtual ~Device() = default;

    /** Write a command to the debugFs entry "name" with the given input buffer.
     *
     * @param[in] name of the debug fs entry.
     * @param[in] bufferInput command to write.
     *
     * @return bytes writen to the debugfs entry
     * @throw Device::Exception in case of write error.
     */
    virtual ssize_t commandWrite(const std::string &name, const util::Buffer &bufferInput) = 0;

    /** read a command to the debugFs entry "name" with the given input buffer as a command
     * and store the result of the read command in the output buffer.
     *
     * @param[in] name of the debug fs entry.
     * @param[in] bufferInput command to write.
     * @param[out] bufferOutput result of the read command.
     *
     * @throw Device::Exception in case of read error.
     */
    virtual void commandRead(const std::string &name, const util::Buffer &bufferInput,
                             util::Buffer &bufferOutput) = 0;

private:
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
};
}
}
}
