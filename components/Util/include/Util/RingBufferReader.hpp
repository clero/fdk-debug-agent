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
#include "Util/Exception.hpp"

#include <functional>
#include <string>
#include <cstddef>
#include <cstdint>

namespace debug_agent
{
namespace util
{

/** Class to abstract a ring buffer to provide safe (ie bound checked) method.
 *
 * It can only be read as it abstracts a ring-buffer which content is controlled by another party.
 * @note Currently the ring buffer contains bytes. Consider parametrizing the element_type.
 *
 * Class invariant in normal state:
 *                   getProducerPosition() >= mConsumerPosition (no underflow)
 *                   getProducerPosition() - mConsumerPosition <= mSize (no overflow)
 * If any of those invariants are broken, the queue will throw on any mutable operation.
 */
class RingBufferReader
{
public:
    using Exception = util::Exception<RingBufferReader>;

    /** Type of the items in the ring buffer.
     * The ring buffer is manipulated as an array of bytes.
     */
    using Sample = volatile uint8_t;

    /** The type to implement the linear position in the ring buffer.
     *
     * Linear position is not a size_t because the ring buffer loops around its bounds. Thus may be
     * writing more than the max of size_t as size_t is only require to be able to contain the size
     * of the biggest byte array allocatable).
     *
     * Note that position overflow is not checked as it should never happen. It would require
     * writing > 10,000,000TB of data for a 64 bit LinearPosition...
     */
    using LinearPosition = uint64_t;

    /** Callable that must return the current producer linear position in the ring buffer. This
     * position MUST be monotonically increasing. */
    using GetProducerLinearPosition = std::function<LinearPosition(void)>;

    /** Constructor
     *
     * @param bufferStart Pointer to the initial position of the ringbuffer
     * @param size The ringbuffer's size
     * @param getProducerLinearPosition The monotonic position producer
     */
    RingBufferReader(Sample *bufferStart, size_t size,
                     GetProducerLinearPosition getProducerLinearPosition)
        : mStart(bufferStart), mSize(size),
          mGetProducerLinearPosition(std::move(getProducerLinearPosition))
    {
    }

    RingBufferReader(RingBufferReader &&) = default;
    RingBufferReader(const RingBufferReader &) = delete;
    RingBufferReader &operator=(const RingBufferReader &) = delete;
    /** Could be implemented, but there is no need for it. */
    RingBufferReader &operator=(RingBufferReader &&) = delete;

    size_t getSize() const { return mSize; }

    /** Read available bytes and append them to the buffer.
     *
     * @param[out] buffer Read data will be copied in the end of this buffer on success.
     *                    Left unmodified on failure.
     * @throw Exception if an unrecoverable error happen such as an overflow happened.
     *                  Ie producerPos - consumerPos > size
     */
    void readAvailable(Buffer &buffer)
    {
        std::size_t producerPosition = mGetProducerLinearPosition();
        if (producerPosition < mConsumerPosition) {
            throw Exception("Driver has returned a non linear position: " +
                            std::to_string(producerPosition));
        }

        std::size_t available = producerPosition - mConsumerPosition;
        if (available > 0) {

            if (available > mSize) {
                throw Exception("Producer has written over consumer position.");
            }

            // Get the required data
            unsafeCopy(available, buffer);

            // Read succeeded, commit changes
            mConsumerPosition += available;
        }
    }

private:
    const Sample *getConsumerPosition() const { return begin() + getConsumerOffset(); }
    size_t getConsumerOffset() const { return mConsumerPosition % mSize; }
    const Sample *begin() const { return mStart; }
    const Sample *end() const { return mStart + mSize; }

    void unsafeCopy(size_t size, Buffer &buffer) const
    {
        buffer.reserve(buffer.size() + size); // Avoid multiple buffer growth
        if (size <= mSize - getConsumerOffset()) {
            // Reading `size` will not buffer overflow, read the requested size
            buffer.insert(buffer.end(), getConsumerPosition(), getConsumerPosition() + size);
        } else {
            // Reading `size` will buffer overflow,
            // instead read to the end of the ring buffer
            buffer.insert(buffer.end(), getConsumerPosition(), end());
            // then read the rest at the beginning.
            buffer.insert(buffer.end(), begin(), getConsumerPosition() - (mSize - size));
        }
    }

    const Sample *mStart;
    const size_t mSize;
    GetProducerLinearPosition mGetProducerLinearPosition;

    LinearPosition mConsumerPosition = 0;
    LinearPosition mProducerPosition = 0;
};
}
}
