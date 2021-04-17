// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_REQUEST_HPP
#define BMC_REQUEST_HPP

#include <memory>

#include <fastcgi++/http.hpp>

namespace app
{
namespace core
{

using namespace Fastcgipp;
using namespace Fastcgipp::Http;

class IRequest
{
  public:
    virtual ~IRequest() = default;

    virtual const Environment<char>& environment() const = 0;
    virtual const Environment<char>& environment() = 0;
    virtual bool parseBody() = 0;
};

class Request : public IRequest
{
  public:
    explicit Request() = delete;

    Request(const Environment<char>& argEnv) : env(argEnv){};
    Request(const Request&) = delete;
    Request(const Request&&) = delete;

    Request& operator=(const Request&) = delete;
    Request& operator=(const Request&&) = delete;

    ~Request() = default;

    const Environment<char>& environment() const override { return env; };
    const Environment<char>& environment() override { return env; };

    bool parseBody() override;
  private:
    const Http::Environment<char>& env;
};


using RequestUni = std::unique_ptr<IRequest>;
using RequestPtr = std::shared_ptr<IRequest>;

} // namespace core

} // namespace app

#endif //! BMC_REQUEST_HPP
