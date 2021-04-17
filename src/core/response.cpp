
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/response.hpp>

#include <logger/logger.hpp>


namespace app
{
namespace core
{

size_t Response::totalSize() const
{
    return internalBuffer.length();
}

const IResponse::IStatus& Response::getStatus()
{
    return this->status;
}

void Response::setStatus(const IStatus& status)
{
    this->status = std::move(status);
}

const std::string Response::getBuffer() const
{
    return internalBuffer;
}

void Response::push(const std::string buffer)
{
    internalBuffer += buffer;
};

void Response::clear()
{
    internalBuffer.clear();
};

} // namespace core
} // namespace app
