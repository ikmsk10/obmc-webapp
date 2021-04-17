
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/request.hpp>

#include <set>
#include <string>

#include <logger/logger.hpp>

namespace app
{
namespace core
{
bool Request::parseBody()
{
    static const std::set<std::string> allowedContentTypes{
        "application/json",
        "text/plain",
    };

    if (environment().postBuffer().empty()) {
        LOG_DEBUG << "No data in POST. Skip parse body";
        return true;
    }

    return !environment().contentType.empty() &&
           allowedContentTypes.contains(environment().contentType);
}

} // namespace core
} // namespace app
