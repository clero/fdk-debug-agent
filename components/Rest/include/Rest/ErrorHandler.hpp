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

#include <Poco/ErrorHandler.h>
#include <Poco/Net/NetException.h>
#include <iostream>
#include <typeinfo>

namespace debug_agent
{
namespace rest
{

/**
 * Implements a Poco ErrorHandler that hides ConnectionAbortedException, because
 * this exception is thrown when a client disconnects from the http server using a
 * keep-alive connection, which is a nominal case. */
class ErrorHandler final : public Poco::ErrorHandler
{
public:
    ErrorHandler() : mBaseErrorHandler(nullptr)
    {
        /* Getting base handler */
        mBaseErrorHandler = Poco::ErrorHandler::get();
        poco_check_ptr(mBaseErrorHandler);

        /* Setting this as current handler*/
        Poco::ErrorHandler::set(this);
    }

    ~ErrorHandler()
    {
        /* Restoring base handler */
        Poco::ErrorHandler::set(mBaseErrorHandler);
    }

    virtual void exception(const Poco::Exception& e) override
    {
        const std::type_info &exceptionType = typeid(e);

        /* comparing the exception type */
        if (exceptionType == typeid(Poco::Net::ConnectionAbortedException)) {

            /* ConnectionAbortedException is a nominal case: ignoring it */
            return;
        }

        /* Calling base handler */
        mBaseErrorHandler->exception(e);
    }

private:
    Poco::ErrorHandler *mBaseErrorHandler;
};

}
}


