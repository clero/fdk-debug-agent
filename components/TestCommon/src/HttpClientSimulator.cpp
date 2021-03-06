/*
 * Copyright (c) 2015, Intel Corporation
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

#include "TestCommon/HttpClientSimulator.hpp"
#include "Util/StringHelper.hpp"
#include "Util/FileHelper.hpp"
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/StreamCopier.h>
#include <cassert>
#include <algorithm>
#include <fstream>

using namespace Poco::Net;

namespace debug_agent
{
namespace test_common
{

/* Translate the Poco status into the HttpClientSimulator::Status enum*/
static HttpClientSimulator::Status translateStatus(HTTPResponse::HTTPStatus status)
{
    switch (status) {
    case HTTPResponse::HTTPStatus::HTTP_OK:
        return HttpClientSimulator::Status::Ok;
    case HTTPResponse::HTTPStatus::HTTP_METHOD_NOT_ALLOWED:
        return HttpClientSimulator::Status::VerbNotAllowed;
    case HTTPResponse::HTTPStatus::HTTP_NOT_FOUND:
        return HttpClientSimulator::Status::NotFound;
    case static_cast<HTTPResponse::HTTPStatus>(423): /* This code is not defined by Poco */
        return HttpClientSimulator::Status::Locked;
    case HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR:
        return HttpClientSimulator::Status::InternalError;
    default:
        // There are around 40 different code in Poco, but only a few are expected
        throw HttpClientSimulator::RequestFailureException(std::string("Invalid http status"));
    }
}

/* Translate the HttpClientSimulator::Verb enum into Poco verb strings*/
const std::string &HttpClientSimulator::toString(Verb verb)
{
    switch (verb) {
    case Verb::Post:
        return HTTPRequest::HTTP_POST;
    case Verb::Get:
        return HTTPRequest::HTTP_GET;
    case Verb::Put:
        return HTTPRequest::HTTP_PUT;
    case Verb::Delete:
        return HTTPRequest::HTTP_DELETE;
    }
    throw RequestFailureException(std::string("Invalid http verb"));
}

std::string HttpClientSimulator::toString(Status s)
{
    switch (s) {
    case Status::Ok:
        return "Ok";
    case Status::NotFound:
        return "NotFound";
    case Status::VerbNotAllowed:
        return "VerbNotAllowed";
    case Status::Locked:
        return "Locked";
    case Status::InternalError:
        return "InternalError";
    }
    throw RequestFailureException(std::string("Invalid http status"));
}

/* String content */

std::string HttpClientSimulator::StringContent::getSubStringSafe(const std::string &str,
                                                                 std::size_t index,
                                                                 std::size_t length)
{
    /* Changing the substring length if it exceeds the input string size */
    std::size_t safeLength;
    if (index + length <= str.length()) {
        safeLength = length;
    } else {
        safeLength = str.length() - index;
    }

    return str.substr(index, safeLength);
}

std::size_t HttpClientSimulator::StringContent::getStringDiffOffset(const std::string &str1,
                                                                    const std::string &str2)
{
    std::size_t minSize = std::min(str1.length(), str2.length());

    for (std::size_t i = 0; i < minSize; i++) {
        if (str1[i] != str2[i])
            return i;
    }
    return minSize;
}

void HttpClientSimulator::StringContent::checkExpected(const util::Buffer &content) const
{
    std::string stringContent(content.begin(), content.end());

    if (stringContent != mExpectedContent) {
        /* The substring that contains difference will not exceed 15 chars */
        static const std::size_t diffLength = 15;

        /* Getting the index of the first different char */
        std::size_t diffIndex = getStringDiffOffset(stringContent, mExpectedContent);

        /* "Got" content substring that contains the difference */
        std::string substringContent = getSubStringSafe(stringContent, diffIndex, diffLength);

        /* "Expected" content substring that contains the difference */
        std::string substringExpectedContent =
            getSubStringSafe(mExpectedContent, diffIndex, diffLength);

        throw RequestFailureException(
            "Wrong response content, got:\n" + stringContent + "\nexpected: '" + mExpectedContent +
            "' at index " + std::to_string(diffIndex) + ": got substring: '" + substringContent +
            "' expected substring: '" + substringExpectedContent + "'");
    }
}

/* File content */
void HttpClientSimulator::FileContent::checkExpected(const util::Buffer &gotContent) const
{
    try {
        auto expectedContent = util::file_helper::readAsBytes(mReferenceFile);

        if (gotContent != expectedContent) {
            std::string gotFile(mReferenceFile + "_got");
            util::file_helper::writeFromBytes(gotFile, gotContent);

            throw RequestFailureException("Wrong response content. See diff between:\n"
                                          "ref: " +
                                          mReferenceFile + "\ngot: " + gotFile);
        }
    } catch (util::file_helper::Exception &e) {
        throw RequestFailureException("File I/O error: " + std::string(e.what()));
    }
}

/* HttpClientSimulator */

void HttpClientSimulator::request(const std::string &uri, Verb verb,
                                  const std::string &requestContent, Status expectedStatus,
                                  const std::string &expectedContentType,
                                  const Content &expectedResponseContent)
{
    util::Buffer requestContentBuffer(requestContent.begin(), requestContent.end());
    request(uri, verb, requestContentBuffer, expectedStatus, expectedContentType,
            expectedResponseContent);
}

void HttpClientSimulator::request(const std::string &uri, Verb verb,
                                  const util::Buffer &requestContent, Status expectedStatus,
                                  const std::string &expectedContentType,
                                  const Content &expectedResponseContent)
{
    HTTPClientSession session(mServer, mPort);

    HTTPRequest request(toString(verb), uri);
    HTTPResponse response;
    util::Buffer responseContent;

    try {
        request.setChunkedTransferEncoding(true);
        request.setKeepAlive(true);

        /* Sending the request header */
        std::ostream &requestStream = session.sendRequest(request);

        /* Sending the request content */
        requestStream.write(reinterpret_cast<const char *>(requestContent.data()),
                            requestContent.size());

        /* Receiving the response header */
        std::istream &responseStream = session.receiveResponse(response);

        /* Receiving the response content */

        responseContent = util::Buffer(std::istreambuf_iterator<char>(responseStream),
                                       std::istreambuf_iterator<char>());
    } catch (NetException &e) {
        throw NetworkException(std::string("Network error: ") + e.what());
    }

    /* Checking status */
    Status status = translateStatus(response.getStatus());
    if (status != expectedStatus) {
        throw RequestFailureException("Wrong status: '" + toString(status) + "' instead of '" +
                                      toString(expectedStatus) + "'");
    }
    /* Checking response content type */
    if (response.getContentType() != expectedContentType) {
        throw RequestFailureException("Wrong content-type: '" + response.getContentType() +
                                      "' instead of '" + expectedContentType + "'");
    }
    /* Checking response content*/
    expectedResponseContent.checkExpected(responseContent);
}
}
}
