// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_ROUTER_HPP
#define BMC_ROUTER_HPP

#include <core/request.hpp>
#include <core/response.hpp>

#include <memory>
#include <string>

namespace app
{
namespace core
{

class IRouteHandler
{
  public:
    virtual void run(const RequestUni& request, ResponseUni& response) = 0;

    virtual ~IRouteHandler() = default;
};

using RouteHandlerPtr = std::shared_ptr<IRouteHandler>;

class GraphqlRouter : public IRouteHandler
{
  public:
    explicit GraphqlRouter(const std::string& iPath) : path(iPath)
    {}

    GraphqlRouter() = default;
    GraphqlRouter(const GraphqlRouter&) = delete;
    GraphqlRouter(const GraphqlRouter&&) = delete;

    GraphqlRouter& operator=(const GraphqlRouter&) = delete;
    GraphqlRouter& operator=(const GraphqlRouter&&) = delete;

    void run(const RequestUni& request, ResponseUni& response) override;

    virtual ~GraphqlRouter() = default;

  private:
    std::string path;
};

class Router
{
    using RoutePattern = std::string;
    using RouteMap = std::map<RoutePattern, RouteHandlerPtr>;

  public:
    explicit Router(const RequestUni& request);

    Router() = delete;
    Router(const Router&) = delete;
    Router(const Router&&) = delete;

    Router& operator=(const Router&) = delete;
    Router& operator=(const Router&&) = delete;

    virtual ~Router() = default;

    const ResponseUni& getResponse() const
    {
        return responseObject;
    }

    const ResponseUni& process();

    template <class THandler, typename... TArg>
    static void registerUri(const std::string pattern, TArg... args)
    {
        static_assert(std::is_base_of_v<IRouteHandler, THandler>,
                      "Unexpected URI handler");
        Router::routerHandlers.emplace(
            pattern, std::make_shared<THandler>(pattern, args...));
    }

  protected:
    const RequestUni& getRequest() const
    {
        return requestObject;
    }

    const RequestUni& getRequest()
    {
        return requestObject;
    }

    ResponseUni& getResponse()
    {
        return responseObject;
    }

  private:
    const RequestUni& requestObject;
    ResponseUni responseObject;

    static RouteMap routerHandlers;
};

} // namespace core

} // namespace app

#endif //! BMC_ROUTER_HPP
