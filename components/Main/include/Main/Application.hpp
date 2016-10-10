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

#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/OptionSet.h>
#include <inttypes.h>

namespace debug_agent
{
namespace main
{

class Application : public Poco::Util::ServerApplication
{
public:
    Application();
    virtual ~Application(){};

protected:
    virtual void defineOptions(Poco::Util::OptionSet &options) override;

    virtual int main(const std::vector<std::string> &) override;

private:
    void usage();

    /* Option handlers */
    void handleHelp(const std::string &name, const std::string &value);
    void handlePort(const std::string &name, const std::string &value);
    void handlePfwConfig(const std::string &name, const std::string &value);
    void handleLogControlOnly(const std::string &name, const std::string &value);
    void handleVerbose(const std::string &name, const std::string &value);
    void handleValidation(const std::string &name, const std::string &value);
    void handleVersion(const std::string &name, const std::string &value);

    struct Config
    {
        bool helpRequested;
        uint32_t serverPort;
        std::string pfwConfig;
        bool logControlOnly;
        bool serverIsVerbose;
        bool validationRequested;
        Config()
            : helpRequested(false), serverPort(9090), logControlOnly(false), serverIsVerbose(false),
              validationRequested(false){};
    };

    Config mConfig;
};
}
}
