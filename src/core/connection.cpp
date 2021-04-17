
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/connection.hpp>

#include <http/headers.hpp>
#include <logger/logger.hpp>
#include <core/router.hpp>

namespace app
{
namespace core
{

Connection::Connection() :
    Fastcgipp::Request<char>(maxBodySizeByte), totalBytesRecived(0),
    requestObject()
{
}

void Connection::inHandler(int bytesReceived)
{
    LOG_DEBUG << "In handler";
    totalBytesRecived += static_cast<size_t>(bytesReceived);
    if (!requestObject)
    {
        requestObject = std::make_unique<app::core::Request>(this->environment());
    }
}

bool Connection::inProcessor()
{
    LOG_DEBUG << "Custom 'Content Type' detected";

    if (environment().contentType.empty()) {
        LOG_ALERT << "Unknown Content Type. Immediate close";
        return false;
    }
    LOG_DEBUG << "Content Type: " << environment().contentType;

    return requestObject->parseBody();
}

bool Connection::response()
{
    LOG_DEBUG << "New Connection. Response";
    using namespace Fastcgipp::Http;

    Router router(requestObject);
    auto& resp = router.process();
    writeHeader(resp);

    out << *resp;
    out.flush();

    return true;
}

void Connection::writeHeader(const ResponseUni& responsePointer)
{
    using namespace app::http;

    decltype(auto) status = responsePointer->getStatus();

    out << headerStatus(status.getCode()) << std::endl
        << header(headers::contentType, content_types::applicationJson)
        << std::endl
        << header(headers::contentLength, std::to_string(responsePointer->totalSize()))
        << std::endl
        << header(headers::date, Connection::getDate()) << std::endl;

    out << std::endl;
}

/**
 * @brief
 *
 * FIXME(IK) move to the standalone file, e.g DateUtils.hpp
 *
 * @return const std::string
 */
const std::string Connection::getDate()
{
    std::string dateStr;
    time_t lastTimeT = time(nullptr);
    tm myTm{};

    gmtime_r(&lastTimeT, &myTm);

    dateStr.resize(100);
    size_t dateStrSz =
        strftime(&dateStr[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &myTm);
    dateStr.resize(dateStrSz);

    return dateStr;
}

} // namespace entity

} // namespace app
