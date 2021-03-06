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

#include "cAVS/Linux/MockedCompressDevice.hpp"
#include "Util/AssertAlways.hpp"
#include <cassert>

using namespace debug_agent::util;

namespace debug_agent
{
namespace cavs
{
namespace linux
{

void MockedCompressDevice::basicMockedOperation(const std::string &func)
{
    checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::lock_guard<std::mutex> locker(mMutex);

    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }
    CompressOperationEntryPtr entryPtr(std::move(mEntries.front()));
    if (entryPtr == nullptr) {
        failure("Wrong CompressDevice method, invalid command.");
    }
    if (func != entryPtr->getOpName()) {
        failure("Wrong CompressDevice method, expecting " + func + ", got " +
                entryPtr->getOpName() + " command.");
    }
    mEntries.pop();

    if (entryPtr->isFailing()) {
        throw Exception("error during compress " + func + ": error#MockDevice");
    }
}

/** below are pure virtual function of Device interface */
void MockedCompressDevice::open(Mode /*mode*/, compress::Role /*role*/,
                                compress::Config & /*config*/)
{
    {
        std::lock_guard<std::mutex> locker(mMutex);
        mIsReady = true;
    }
    basicMockedOperation(__func__);
}

void MockedCompressDevice::close() noexcept
{
    std::lock_guard<std::mutex> locker(mMutex);
    mIsRunning = false;
    mIsReady = false;
}

bool MockedCompressDevice::wait(int /*waitMs*/)
{
    checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::unique_lock<std::mutex> locker(mMutex);

    if (not mIsRunning) {
        /** The device has been stopped before the read/write thread could call wait.
         * It means wait shall fail to return. Find the wait mock within the entries list
         * as the order may not be correct.
         */
        throw IoException();
    }
    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }

    CompressOperationEntryPtr entryPtr = std::move(mEntries.front());
    CompressWaitEntry *entry = dynamic_cast<CompressWaitEntry *>(entryPtr.get());
    if (entry == nullptr) {
        failure("Wrong CompressDevice method, expecting wait.");
    }
    mEntries.pop();
    if (entry->getSyncWait() != nullptr) {
        // External sync
        entry->getSyncWait()->waitUntilUnblock();
    } else if (entry->getTimeToWaitInMs() < 0) {
        // Blocked until compress device is stopped (internal sync)
        mCondVar.wait(locker);
    }
    if (entry->isFailing()) {
        throw Exception("error during compress wait: error#MockDevice");
    }
    return entry->getReply();
}

void MockedCompressDevice::start()
{
    {
        std::unique_lock<std::mutex> locker(mMutex);
        mIsRunning = true;
    }
    basicMockedOperation(__func__);
}

void MockedCompressDevice::stop()
{
    std::unique_lock<std::mutex> locker(mMutex);
    if (mIsRunning) {
        mCondVar.notify_one();
    }
    mIsRunning = false;
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }
    /** As stop may be called from a different context than the read/write thread, the order of
     * stop and wait mock is not garanted.
     */
    while (__func__ != mEntries.front().get()->getOpName()) {
        // Removing the entry
        mEntries.pop();
        /* Checking that the test vector is not already consumed */
        if (consumed()) {
            failure("MockedCompressDevice vector already consumed.");
        }
    }
    assert(__func__ == mEntries.front().get()->getOpName());

    CompressOperationEntryPtr entryPtr(std::move(mEntries.front()));
    if (entryPtr == nullptr) {
        failure("Wrong CompressDevice method, invalid command.");
    }
    mEntries.pop();

    if (entryPtr->isFailing()) {
        throw Exception("error during compress stop error#MockDevice");
    }
}

bool MockedCompressDevice::isRunning() const noexcept
{
    std::unique_lock<std::mutex> locker(mMutex);
    return mIsRunning;
}

bool MockedCompressDevice::isReady() const noexcept
{
    std::unique_lock<std::mutex> locker(mMutex);
    return mIsReady;
}

size_t MockedCompressDevice::write(const util::Buffer &inputBuffer)
{
    checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::lock_guard<std::mutex> locker(mMutex);

    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }
    CompressOperationEntryPtr entryPtr(std::move(mEntries.front()));
    const CompressWriteEntry *entry = dynamic_cast<CompressWriteEntry *>(entryPtr.get());
    if (entry == nullptr) {
        failure("Wrong CompressDevice method, expecting write.");
    }
    mEntries.pop();

    if (entry->isFailing()) {
        throw Exception("error during compress read error#MockDevice");
    }
    /* Checking input buffer content */
    compareBuffers("Input buffer", inputBuffer, entry->getExpectedInputBuffer());
    return entry->getReturnedSize();
}

size_t MockedCompressDevice::read(util::Buffer &outputBuffer)
{
    checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::lock_guard<std::mutex> locker(mMutex);

    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }
    CompressOperationEntryPtr entryPtr(std::move(mEntries.front()));
    const CompressReadEntry *entry = dynamic_cast<CompressReadEntry *>(entryPtr.get());
    if (entry == nullptr) {
        failure("Wrong CompressDevice method, expecting read.");
    }
    mEntries.pop();

    if (entry->isFailing()) {
        throw Exception("error during compress read error#MockDevice");
    }
    outputBuffer = entry->getReturnedOutputBuffer();
    return entry->getReturnedSize();
}

std::size_t MockedCompressDevice::getAvailable()
{
    checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::lock_guard<std::mutex> locker(mMutex);

    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        failure("MockedCompressDevice vector already consumed.");
    }
    CompressOperationEntryPtr entryPtr(std::move(mEntries.front()));
    const CompressAvailEntry *entry = dynamic_cast<CompressAvailEntry *>(entryPtr.get());
    if (entry == nullptr) {
        failure("Wrong method, expecting another command.");
    }
    mEntries.pop();

    if (entry->isFailing()) {
        throw Exception("error during compress getAvailable error#MockDevice");
    }
    return entry->getReturnedSize();
}

std::size_t MockedCompressDevice::getBufferSize() const
{
    MockedCompressDevice *me = const_cast<MockedCompressDevice *>(this);
    me->checkNonFailure();
    /* Several threads can call this method, so protecting against it.
     *
     * Note: although this mutex protects against concurrent calls,
     *       no concurrent calls should happen because this leads to randomize the call order
     *       which won't match probably the test vector.
     */
    std::lock_guard<std::mutex> locker(mMutex);

    /* Checking that the test vector is not already consumed */
    if (consumed()) {
        me->failure("MockedCompressDevice vector already consumed.");
    }
    CompressOperationEntryPtr entryPtr(std::move(me->mEntries.front()));
    const CompressGetBufferSizeEntry *entry =
        dynamic_cast<CompressGetBufferSizeEntry *>(entryPtr.get());
    if (entry == nullptr) {
        me->failure("Wrong method, expecting another command.");
    }
    me->mEntries.pop();

    if (entry->isFailing()) {
        throw Exception("error during compress getBufferSize error#MockDevice");
    }
    return entry->getReturnedSize();
}

MockedCompressDevice::~MockedCompressDevice()
{
    if (!consumed()) {
        mLeftoverCallback();
    }
}

bool MockedCompressDevice::consumed() const
{
    return mEntries.empty();
}

void MockedCompressDevice::compareBuffers(const std::string &bufferName,
                                          const Buffer &candidateBuffer,
                                          const Buffer &expectedBuffer)
{
    /* Checking size */
    if (candidateBuffer.size() != expectedBuffer.size()) {
        entryFailure(bufferName + " candidate with size " + std::to_string(candidateBuffer.size()) +
                     " differs from required size: " + std::to_string(expectedBuffer.size()));
    }

    /* Checking buffer content */
    if (candidateBuffer != expectedBuffer) {
        entryFailure(bufferName + " content is not the expected one.");
    }
}
}
}
}
