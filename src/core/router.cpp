
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/router.hpp>
#include <nlohmann/json.hpp>

namespace app
{
namespace core
{
void GraphqlRouter::run(const RequestUni& request, ResponseUni& response)
{
    nlohmann::json data {
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"MessageId", "Base.1.8.1.ResourceInUse"},
        {"Message", "The change to the requested resource failed because "
                    "the resource is in use or in transition."},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageSeverity", "Warning"},
        {"Resolution", "Remove the condition and resubmit the request if "
                       "the operation failed."}};
    response->push(data.dump());

    response->setStatus(status::Success());
}

Router::RouteMap Router::routerHandlers;

Router::Router(const RequestUni& request) :
    requestObject(request),
    responseObject(std::make_unique<app::core::Response>())
{}

const ResponseUni& Router::process()
{
    auto handler = routerHandlers.find(getRequest()->environment().requestUri);
    if (handler == routerHandlers.end())
    {
        getResponse()->setStatus(status::NotFound());
        return std::move(getResponse());
    }

    handler->second->run(getRequest(), getResponse());

    return getResponse();
}

} // namespace core
} // namespace app
