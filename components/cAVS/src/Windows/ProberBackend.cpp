/*
 * Copyright (c) 2016, Intel Corporation
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

#include "cAVS/Windows/ProberBackend.hpp"
#include "cAVS/Windows/IoctlHelpers.hpp"
#include "cAVS/Windows/Probe/ExtractionInputStream.hpp"
#include "cAVS/Windows/Probe/InjectionOutputStream.hpp"

namespace debug_agent
{
namespace cavs
{
namespace windows
{

using ioctl_helpers::ioctl;

static const std::map<Prober::ProbePurpose, driver::ProbePurpose> purposeConversion = {
    {Prober::ProbePurpose::Inject, driver::ProbePurpose::InjectPurpose},
    {Prober::ProbePurpose::Extract, driver::ProbePurpose::ExtractPurpose},
    {Prober::ProbePurpose::InjectReextract, driver::ProbePurpose::InjectReextractPurpose}};

static const std::map<ProberBackend::State, driver::ProbeState> stateConversion = {
    {ProberBackend::State::Idle, driver::ProbeState::ProbeFeatureIdle},
    {ProberBackend::State::Owned, driver::ProbeState::ProbeFeatureOwned},
    {ProberBackend::State::Allocated, driver::ProbeState::ProbeFeatureAllocated},
    {ProberBackend::State::Active, driver::ProbeState::ProbeFeatureActive}};

driver::ProbePointConfiguration ProberBackend::toWindows(
    const cavs::Prober::SessionProbes &probes, const ProberBackend::EventHandles &eventHandles)
{
    if (probes.size() != driver::maxProbes) {
        throw Exception("Expected to receive " + std::to_string(driver::maxProbes) +
                        " probe configurations to set. (Actually received " +
                        std::to_string(probes.size()));
    }

    // Translate the high-level data into driver-specific structures
    std::vector<driver::ProbePointConnection> connections;
    std::size_t probeIndex = 0;
    for (const auto &probe : probes) {
        connections.emplace_back(toWindows(probe.enabled), probe.probePoint,
                                 toWindows(probe.purpose),
                                 eventHandles.injectionHandles[probeIndex]->handle());

        ++probeIndex;
    }

    driver::ProbePointConfiguration toDriver;
    toDriver.extractionBufferCompletionEventHandle = eventHandles.extractionHandle->handle();
    std::copy_n(connections.begin(), driver::maxProbes, toDriver.probePointConnection);
    return toDriver;
}

driver::ProbeState ProberBackend::toWindows(const ProberBackend::State &from)
{
    try {
        return stateConversion.at(from);
    } catch (std::out_of_range &) {
        throw Exception("Wrong state value (" + std::to_string(static_cast<uint32_t>(from)) + ").");
    }
}

ProberBackend::State ProberBackend::fromWindows(const driver::ProbeState &from)
{
    for (auto &candidate : stateConversion) {
        if (candidate.second == from) {
            return candidate.first;
        }
    }
    throw Exception("Wrong state value (" + std::to_string(static_cast<uint32_t>(from)) + ").");
}

driver::ProbePurpose ProberBackend::toWindows(const cavs::Prober::ProbePurpose &from)
{
    try {
        return purposeConversion.at(from);
    } catch (std::out_of_range &) {
        throw Exception("Wrong purpose value (" + std::to_string(static_cast<uint32_t>(from)) +
                        ").");
    }
}

cavs::Prober::ProbePurpose ProberBackend::fromWindows(const driver::ProbePurpose &from)
{
    for (auto &candidate : purposeConversion) {
        if (candidate.second == from) {
            return candidate.first;
        }
    }
    throw Exception("Wrong purpose value (" + std::to_string(static_cast<uint32_t>(from)) + ").");
}

BOOL ProberBackend::toWindows(bool value)
{
    return value ? TRUE : FALSE;
}

bool ProberBackend::fromWindows(BOOL value)
{
    switch (value) {
    case TRUE:
        return true;
    case FALSE:
        return false;
    }
    throw Exception("Unknown BOOL value: " + std::to_string(value));
}

void ProberBackend::setStateTransition(State newState, State oldState)
{
    switch (newState) {
    case State::Active:
        startStreaming();
        break;
    // If not active, stop streaming
    case State::Idle:
    case State::Owned:
    case State::Allocated:
        stopStreaming();
    }

    auto tmp = toWindows(newState);
    ioctl<SetState, Exception>(mDevice, tmp);

    if (oldState == State::Idle && newState == State::Owned) {
        /* State is "Owned" : applying cached configuration to the driver */
        setSessionProbes();
    }
}

ProberBackend::ProberBackend(Device &device, const EventHandles &eventHandles)
    : mDevice(device), mEventHandles(eventHandles), mCachedProbeConfiguration(getMaxProbeCount())
{
    for (std::size_t probeIndex = 0; probeIndex < getMaxProbeCount(); ++probeIndex) {
        mExtractionQueues.emplace_back(mQueueSize,
                                       [](const util::Buffer &buffer) { return buffer.size(); });
        mInjectionQueues.emplace_back(mQueueSize);
    }
}

ProberBackend::State ProberBackend::getState()
{
    // Initialize with an illegal value in case the driver fails to overwrite it.
    driver::ProbeState state{static_cast<driver::ProbeState>(-1)};
    ioctl<GetState, Exception>(mDevice, state);

    return fromWindows(state);
}

void ProberBackend::checkProbeId(ProbeId probeId) const
{
    if (probeId.getValue() >= getMaxProbeCount()) {
        throw Exception("Invalid probe index: " + std::to_string(probeId.getValue()));
    }
}

void ProberBackend::setSessionProbes(
    const cavs::Prober::SessionProbes &probes,
    const cavs::Prober::InjectionSampleByteSizes &injectionSampleByteSizes)
{
    std::lock_guard<std::mutex> guard(mProbeConfigMutex);

    // Caching info needed to start probing
    mCachedProbeConfiguration = probes;
    mCachedInjectionSampleByteSizes = injectionSampleByteSizes;
}

cavs::Prober::SessionProbes ProberBackend::getSessionProbes()
{
    driver::ProbePointConfiguration from;
    memset(&from, 0xFF, sizeof(from));
    ioctl<GetProbePointConfiguration, Exception>(mDevice, from);

    cavs::Prober::SessionProbes result;
    for (const auto &connection : from.probePointConnection) {
        result.emplace_back(fromWindows(connection.enabled), connection.probePointId,
                            fromWindows(connection.purpose));
    }
    return result;
}

void ProberBackend::setSessionProbes()
{
    std::lock_guard<std::mutex> guard(mProbeConfigMutex);

    // Calling the ioctl
    ioctl<SetProbePointConfiguration, Exception>(
        mDevice, toWindows(mCachedProbeConfiguration, mEventHandles));
}

std::unique_ptr<util::Buffer> ProberBackend::dequeueExtractionBlock(ProbeId probeIndex)
{
    checkProbeId(probeIndex);
    return mExtractionQueues[probeIndex.getValue()].remove();
}

bool ProberBackend::enqueueInjectionBlock(ProbeId probeIndex, const util::Buffer &buffer)
{
    checkProbeId(probeIndex);

    // Blocks if the injection queue is full
    return mInjectionQueues[probeIndex.getValue()].writeBlocking(buffer.data(), buffer.size());
}

driver::RingBuffersDescription ProberBackend::getRingBuffers()
{
    using RingBuffer = driver::RingBufferDescription;
    using RingBuffers = driver::RingBuffersDescription;

    driver::RingBuffersDescription from;
    memset(&from, 0xFF, sizeof(from));
    ioctl<GetRingBuffersDescription, Exception>(mDevice, from);

    return from;
}

size_t ProberBackend::getExtractionRingBufferLinearPosition()
{
    uint64_t from;
    memset(&from, 0xFF, sizeof(from));
    ioctl<GetExtractionRingBufferPosition, Exception>(mDevice, from);
    return size_t{from};
}

size_t ProberBackend::getInjectionRingBufferLinearPosition(ProbeId probeId)
{
    checkProbeId(probeId);

    uint64_t from;
    memset(&from, 0xFF, sizeof(from));

    ULONG paramId = driver::ProbeFeatureParameter::INJECTION_BUFFER0_STATUS +
                    probeId.getValue(); // According to the SwAS
    ioctl<decltype(from), Exception>(mDevice, driver::IoCtlType::TinyGet, mProbeFeature, paramId,
                                     from);
    return size_t{from};
}

void ProberBackend::startStreaming()
{
    auto ringBuffers = getRingBuffers();

    auto result = cavs::Prober::getActiveSession(mCachedProbeConfiguration);
    auto &extractionProbes = result.first;
    auto &injectionProbes = result.second;

    try {
        if (!extractionProbes.empty()) {

            // opening queues of the active probes, and collecting probe point id
            ProbeExtractor::ProbePointMap probePointMap;
            for (auto probeId : extractionProbes) {
                mExtractionQueues[probeId.getValue()].open();

                dsp_fw::ProbePointId probePointId =
                    mCachedProbeConfiguration[probeId.getValue()].probePoint;
                // Checking that the probe point id is not already in the map
                if (probePointMap.find(probePointId) != probePointMap.end()) {
                    throw Exception("Two active extraction probes have the same probe point id: " +
                                    probePointId.toString());
                }

                probePointMap[probePointId] = probeId;
            }

            // starting extractor
            auto inputStream = std::make_unique<probe::ExtractionInputStream>(
                *mEventHandles.extractionHandle,
                util::RingBufferReader(ringBuffers.extractionRBDescription.startAdress,
                                       ringBuffers.extractionRBDescription.size,
                                       [this] { return getExtractionRingBufferLinearPosition(); }));
            mExtractor = std::make_unique<ProbeExtractor>(mExtractionQueues, probePointMap,
                                                          std::move(inputStream));
        }

        // opening queues of the active probes and creating injectors
        for (auto probeId : injectionProbes) {
            mInjectionQueues[probeId.getValue()].open();

            // Finding associated sample byte size
            auto it = mCachedInjectionSampleByteSizes.find(probeId);
            if (it == mCachedInjectionSampleByteSizes.end()) {
                throw Exception("Sample byte size not found for injection probe id " +
                                std::to_string(probeId.getValue()));
            }
            if (it->second == 0) {
                throw Exception("Sample byte size must be greater than 0 for injection probe id " +
                                std::to_string(probeId.getValue()));
            }

            auto &rbDesc = ringBuffers.injectionRBDescriptions[probeId.getValue()];

            // Creating injector
            auto outputStream = std::make_unique<probe::InjectionOutputStream>(
                *mEventHandles.injectionHandles[probeId.getValue()],
                util::RingBufferWriter(rbDesc.startAdress, rbDesc.size, [this, probeId] {
                    return getInjectionRingBufferLinearPosition(probeId);
                }));
            mInjectors.emplace_back(std::move(outputStream), mInjectionQueues[probeId.getValue()],
                                    it->second);
        }
    } catch (std::exception &e1) {
        try {
            std::cerr << "Cancelling starting because: " << e1.what() << std::endl;
            stopStreaming();
        } catch (Exception &e2) {
            std::cerr << "Unable to cancel starting : " << e2.what() << std::endl;
        }
    }
}

void ProberBackend::stopStreaming()
{
    // Deleting extractor (the packet producer)
    mExtractor.reset();

    // deleting injectors
    mInjectors.clear();

    // then closing the queues to make wakup listening threads
    for (std::size_t probeIndex = 0; probeIndex < getMaxProbeCount(); ++probeIndex) {
        mExtractionQueues[probeIndex].close();
        mInjectionQueues[probeIndex].close();
    }
}
}
}
}
