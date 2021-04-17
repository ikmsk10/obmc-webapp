// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef BMC_APPLICATION_HPP
#define BMC_APPLICATION_HPP

#include <memory>

#include <fastcgi++/manager.hpp>
#include <logger/logger.hpp>

namespace app
{
namespace core
{

class Application
{
public:
	Application() = default;
    virtual ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application& old) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&& old) = delete;

    /**
     * @brief Configure Web Application.
     * @throw Invalid Argument
     */
    void configure();

    /**
     * @brief Start Web Application
     * @throw std::exception
     */
    void start();

    static void registerAllRoutes();
};


} // namespace entity

} // namespace app



#endif //!BMC_APPLICATION_HPP
