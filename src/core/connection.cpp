
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/connection.hpp>
#include <http/headers.hpp>
#include <logger/logger.hpp>

namespace app
{
namespace core
{

Connection::Connection() :
    Fastcgipp::Request<char>(maxBodySizeByte), totalBytesRecived(0), request(),
    router()
{}

void Connection::inHandler(int bytesReceived)
{
    LOG_DEBUG << "In handler, bytes count=" << bytesReceived;
    totalBytesRecived += static_cast<size_t>(bytesReceived);
    if (!request)
    {
        request = std::make_shared<app::core::Request>(this->environment());
    }
}

bool Connection::inProcessor()
{
    LOG_DEBUG << "Custom 'Content Type' detected";

    if (environment().contentType.empty())
    {
        LOG_ALERT << "Unknown Content Type. Immediate close";
        return false;
    }
    LOG_DEBUG << "Content Type: " << environment().contentType;
    if (!request)
    {
        LOG_CRITICAL << "The request not initialized.";
        return false;
    }

    if (!router)
    {
        router = std::make_unique<app::core::Router>(this->request);
    }

    // Processing the 'preHandlers' in the 'inProcessor', because after that
    // procedure the Post Data Buffer will be cleared.
    return request->validate() && router->preHandler();
}

bool Connection::response()
{
    LOG_DEBUG << "New Connection. Response";
    using namespace Fastcgipp::Http;

    if (!router)
    {
        LOG_CRITICAL << "The route handler not initialized.";
        return false;
    }

    LOG_DEBUG << "Process Route.";
    decltype(auto) responsePtr = router->process();
    LOG_DEBUG << "Write Headers.";
    writeHeader(responsePtr);
    LOG_DEBUG << "Write response to out.";

    out << *responsePtr;
    LOG_DEBUG << "Immediate flush data.";
    out.flush();

    return true;
}

void Connection::writeHeader(const ResponseUni& responsePointer)
{
    using namespace app::http;

    decltype(auto) status = responsePointer->getStatus();

    LOG_DEBUG << "Write status header." << static_cast<int>(status);
    out << headerStatus(status) << std::endl;

    responsePointer->setHeader(headers::contentType,
                               content_types::applicationJson);
    responsePointer->setHeader(headers::contentLength,
                               std::to_string(responsePointer->totalSize()));
    responsePointer->setHeader(headers::date,
                               app::helpers::utils::getFormattedCurrentDate());

    out << responsePointer->getHeaders();
}

} // namespace core

} // namespace app
