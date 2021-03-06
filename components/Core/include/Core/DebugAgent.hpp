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

#include "Core/TypeModel.hpp"
#include "Core/InstanceModel.hpp"
#include "Core/ParameterDispatcher.hpp"
#include "cAVS/System.hpp"
#include "Rest/Server.hpp"
#include "Util/Locker.hpp"
#include "ParameterSerializer/ParameterSerializer.hpp"
#include <inttypes.h>
#include <memory>

namespace debug_agent
{
namespace core
{

/** The debug agent application class */
class DebugAgent final
{
public:
    /** @throw DebugAgent::Exception */
    DebugAgent(const cavs::DriverFactory &driverFactory, uint32_t port,
               const std::string &pfwConfig, bool serverIsVerbose = false,
               bool validationRequested = false);
    ~DebugAgent();

    struct Exception : std::logic_error
    {
        using std::logic_error::logic_error;
    };

private:
    DebugAgent(const DebugAgent &) = delete;
    DebugAgent &operator=(const DebugAgent &) = delete;

    std::shared_ptr<TypeModel> createTypeModel();
    static std::shared_ptr<ifdk_objects::instance::System> createSystemInstance();
    std::unique_ptr<rest::Dispatcher> createDispatcher();
    static std::vector<std::shared_ptr<ParameterApplier>> createParamAppliers(
        cavs::System &system,
        util::Locker<parameter_serializer::ParameterSerializer> &paramSerializer);

    cavs::System mSystem;
    std::shared_ptr<TypeModel> mTypeModel;
    std::shared_ptr<ifdk_objects::instance::System> mSystemInstance;
    util::Locker<std::shared_ptr<InstanceModel>> mInstanceModel;
    util::Locker<parameter_serializer::ParameterSerializer> mParameterSerializer;
    ParameterDispatcher mParamDispatcher;
    rest::Server mRestServer;
};
}
}
