// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_RESPONSE_HPP
#define BMC_RESPONSE_HPP

#include <fastcgi++/http.hpp>
#include <http/headers.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <memory>
#include <string>

namespace app
{
namespace core
{

using namespace Fastcgipp;
using namespace app::http;

/**
 * @brief
 *
 */
class IResponse
{
  public:
    struct IStatus
    {
      public:
        virtual statuses::Code getCode() const = 0;
        virtual const std::string getMessage() const = 0;
        virtual ~IStatus() = default;
    };
    /**
     * @brief
     *
     * @return size_t
     */
    virtual size_t totalSize() const = 0;
    /**
     * @brief
     *
     * @return std::string
     */
    virtual const std::string getBuffer() const = 0;
    /**
     * @brief
     *
     * @return statuses::Code
     */
    virtual const IStatus& getStatus() = 0;
    /**
     * @brief Set the Status object
     *
     * @param status
     */
    virtual void setStatus(const IStatus& status) = 0;

    /**
     * @brief Destroy the IResponse object
     *
     */
    virtual ~IResponse() = default;

    /**
     * @brief
     *
     * @param buffer
     */
    virtual void push(const std::string) = 0;
    /**
     * @brief
     */
    virtual void clear() = 0;
};

using StatusUni = std::unique_ptr<IResponse::IStatus>;
using StatusPtr = std::shared_ptr<IResponse::IStatus>;

namespace status
{

struct AbstractStatus: public IResponse::IStatus
{
  private:
    const statuses::Code code;
    const std::string message;
  public:
    AbstractStatus(const statuses::Code& inCode, const std::string inMessage = "") :
        code(inCode), message(std::move(inMessage))
    {}

    statuses::Code getCode() const override
    {
        return code;
    }
    const std::string getMessage() const override
    {
        return message;
    }

    ~AbstractStatus() = default;
};

struct Success : public AbstractStatus
{
    Success(): AbstractStatus(statuses::Code::OK) {}
    ~Success() = default;
};

struct NotFound : public AbstractStatus
{
    NotFound(): AbstractStatus(statuses::Code::NotFound) {}
    ~NotFound() = default;
};

struct AccessDenied : public AbstractStatus
{
    AccessDenied(): AbstractStatus(statuses::Code::Forbidden) {}
    ~AccessDenied() = default;
};

struct BadRequest : public AbstractStatus
{
    BadRequest(): AbstractStatus(statuses::Code::BadRequest) {}
    ~BadRequest() = default;
};

struct Unauthorized : public AbstractStatus
{
    Unauthorized(): AbstractStatus(statuses::Code::Unauthorized) {}
    ~Unauthorized() = default;
};

struct InternalError : public AbstractStatus
{
    InternalError(): AbstractStatus(statuses::Code::InternalServerError) {}
    ~InternalError() = default;
};

struct ServiceUnavailable : public AbstractStatus
{
    ServiceUnavailable(): AbstractStatus(statuses::Code::ServiceUnavailable) {}
    ~ServiceUnavailable() = default;
};

} // namespace status

class Response : public IResponse
{
  public:
    explicit Response(): status(app::core::status::Success()) { };
    Response(const Response&) = delete;
    Response(const Response&&) = delete;

    Response& operator=(const Response&) = delete;
    Response& operator=(const Response&&) = delete;
    /**
     * @brief Construct a new Response object
     *
     * @param argEnv
     */
    ~Response() override = default;

    size_t totalSize() const override;

    const std::string getBuffer() const override;

    const IStatus& getStatus() override;

    void setStatus(const IStatus& status) override;

    void push(const std::string) override;

    void clear() override;

  private:
    std::string internalBuffer;
    IStatus&& status;
};

/**
 * @brief
 *
 * @param os
 * @param response
 * @return std::ostream&
 */
static std::ostream& operator<<(std::ostream& os, const IResponse& response)
{
    os << response.getBuffer();
    return os;
}

using ResponseUni = std::unique_ptr<IResponse>;
using ResponsePtr = std::shared_ptr<IResponse>;

} // namespace core

} // namespace app

#endif //! BMC_RESPONSE_HPP
