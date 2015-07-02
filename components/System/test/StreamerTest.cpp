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
#include <System/Streamer.hpp>
#include <TestCommon/TestHelpers.hpp>
#include "catch.hpp"
#include <ostream>
#include <string>
#include <stdexcept>

using namespace debug_agent::system;

/**
 * StreamerTest is a subclass of Streamer which allows to test the Streamer class.
 * It generates a simple stream which streams out a given number of unsigned integer from 0.
 * The integers are streamed out formatted. Once the number of iterations is reached, the
 * StreamerTest stops the stream.
 * The StreamerTest also provides the expected stream to be compared with the one it will generate
 * through Streamer.
 */
class StreamerTest: public Streamer
{
public:
    StreamerTest(size_t iterations):
        mIterations(iterations),
        mIteration(0),
        mExpectedStream(),
        mFirstCalled(false)
    {
        // Compute expected stream
        for (size_t i = 0; i < iterations; ++i) {

            mExpectedStream << i;
        }
    }

    void streamFirst(std::ostream &os) override
    {
        // Shall be called only once, before streamNext
        CHECK(mIteration == 0);
        CHECK(mFirstCalled == false);
        mFirstCalled = true;
    }

    bool streamNext(std::ostream &os) override
    {
        if (mIteration < mIterations) {

            // Test streams is just mIteration streamed out (formatted)
            os << mIteration++;
            return true;
        } else {

            // Test stream end
            return false;
        }
    }

    const std::stringstream &getExpectedStream()
    {
        return mExpectedStream;
    }

    const size_t mIterations;
    size_t mIteration;
    std::stringstream mExpectedStream;
    bool mFirstCalled;
};

/**
 * This test runs the StreamerTest::operator<<() in order to check that its Streamer parent class
 * will exercise it as expected. The streams generated by StreamerTest::operator<<() is then
 * compared with the expected one.
 */
TEST_CASE("Test stream", "[stream]")
{
    StreamerTest streamer(50);
    std::stringstream outStream;

    CHECK_THROWS_MSG(outStream << streamer, "End of stream");

    CHECK(outStream.str() == streamer.getExpectedStream().str());
}