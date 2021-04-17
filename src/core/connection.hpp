// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_CONNECTION_HPP
#define BMC_CONNECTION_HPP

#include <fastcgi++/request.hpp>
#include <core/response.hpp>
#include <core/request.hpp>

#include <config.h>

namespace app
{
namespace core
{

class Connection :public Fastcgipp::Request<char>
{
    static constexpr const size_t maxBodySizeByte = (HTTP_REQ_BODY_LIMIT_MB << 20U);

  public:
    Connection();

    Connection(const Connection&) = delete;
    Connection(const Connection&&) = delete;

    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;

    ~Connection() = default;

  protected:
    void inHandler(int) override;
    bool response() override;
    bool inProcessor() override;

  private:
    void writeHeader(const ResponseUni& responsePointer);
    static const std::string getDate();
    size_t totalBytesRecived;

    RequestUni requestObject;
    ResponseUni responseObject;
};


} // namespace entity

} // namespace app



#endif //!BMC_CONNECTION_HPP
