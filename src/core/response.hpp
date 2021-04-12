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
    // struct IStatus
    // {
    //   public:
    //     virtual statuses::Code getCode() const = 0;
    //     virtual const std::string getMessage() const = 0;
    //     virtual ~IStatus() = default;
    // };
    /**
     * @brief
     *
     * @return size_t
     */
    virtual size_t totalSize() const = 0;
    /**
     * @brief get the buffer of body
     *
     * @return std::string
     */
    virtual const std::string& getBody() const = 0;
    /**
     * @brief Set the HTTP response code
     *
     * @return statuses::Code
     */
    virtual const statuses::Code& getStatus() = 0;
    /**
     * @brief Set the Status object
     *
     * @param status
     */
    virtual void setStatus(const statuses::Code&) = 0;

    /**
     * @brief Set the HTTP Header
     *
     * @param headerName - name of header
     * @param headerValue - value of header
     */
    virtual void setHeader(const std::string&, const std::string&) = 0;

    /**
     * @brief Get the Headers buffer
     *
     * @return const std::string& the buffer of all headers.
     */
    virtual const std::string getHeaders() const = 0;
    /**
     * @brief Destroy the IResponse object
     *
     */
    virtual ~IResponse() = default;

    /**
     * @brief Add the response data which will be written to the output fastCGI
     * pipe
     *
     * @param buffer data
     */
    virtual void push(const std::string&) = 0;
    /**
     * @brief clear the internal output buffer
     */
    virtual void clear() = 0;

    /**
     * @brief
     *
     * @param os
     * @param response
     * @return std::ostream&
     */
    friend std::ostream& operator<<(std::ostream& os, const IResponse& response)
    {
        os << response.getBody();
        return os;
    }
};

// using StatusUni = std::unique_ptr<IResponse::IStatus>;
// using StatusPtr = std::shared_ptr<IResponse::IStatus>;

// namespace status
// {

// struct AbstractStatus: public IResponse::IStatus
// {
//   private:
//     statuses::Code code;
//     std::string message;
//   public:
//     AbstractStatus(const statuses::Code inCode, const std::string inMessage =
//     "") :
//         code(inCode), message(inMessage)
//     {}

//     statuses::Code getCode() const override
//     {
//         return code;
//     }
//     const std::string getMessage() const override
//     {
//         return message;
//     }

//     virtual ~AbstractStatus() = default;
// };

// struct Success : public AbstractStatus
// {
//     Success(const std::string message = "OK") :
//         AbstractStatus(statuses::Code::OK, message)
//     {}
//     ~Success() = default;
// };

// // struct NotFound : public IResponse::IStatus
// // {
// //     NotFound(const std::string inMessage = "Not Found") :
// message(inMessage)
// //     {}
// //     ~NotFound() = default;
// //     statuses::Code getCode() const override
// //     {
// //         return statuses:Code::NotFound;
// //     }
// //     const std::string getMessage() const override
// //     {
// //         return message;
// //     }
// // private:
// //     std::string message;
// // };

// struct AccessDenied : public AbstractStatus
// {
//     AccessDenied(const std::string message = "Forbidden") :
//         AbstractStatus(statuses::Code::Forbidden, std::move(message))
//     {}
//     ~AccessDenied() = default;
// };

// struct BadRequest : public AbstractStatus
// {
//     BadRequest(const std::string message = "Bad Request") :
//         AbstractStatus(statuses::Code::BadRequest, std::move(message))
//     {}
//     ~BadRequest() = default;
// };

// struct Unauthorized : public AbstractStatus
// {
//     Unauthorized(const std::string message = "Unauthorized") :
//         AbstractStatus(statuses::Code::Unauthorized, std::move(message))
//     {}
//     ~Unauthorized() = default;
// };

// struct InternalError : public AbstractStatus
// {
//     InternalError(const std::string message = "Internal Server Error") :
//         AbstractStatus(statuses::Code::InternalServerError,
//         std::move(message))
//     {}
//     ~InternalError() = default;
// };

// struct ServiceUnavailable : public AbstractStatus
// {
//     ServiceUnavailable(const std::string message = "Service Temporarily
//     Unavailable") :
//         AbstractStatus(statuses::Code::ServiceUnavailable,
//         std::move(message))
//     {}
//     ~ServiceUnavailable() = default;
// };

// } // namespace status

class Response : public IResponse
{
    static constexpr const char* endHeaderLine = "\r\n";

  public:
    explicit Response() : status(statuses::Code::NotFound){};
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

    const std::string& getBody() const override;

    const statuses::Code& getStatus() override;

    void setStatus(const statuses::Code&) override;

    void setHeader(const std::string&, const std::string&) override;
    const std::string getHeaders() const override;

    void push(const std::string&) override;

    void clear() override;

  private:
    std::string headerBuffer;
    std::string internalBuffer;
    statuses::Code status;
};

using ResponseUni = std::unique_ptr<IResponse>;
using ResponsePtr = std::shared_ptr<IResponse>;

} // namespace core

} // namespace app

#endif //! BMC_RESPONSE_HPP
