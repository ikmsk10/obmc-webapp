
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/connection.hpp>

namespace app
{
namespace core
{

void Application::configure()
{
    // TODO set Threads/Features/APIs
    registerAllRoutes();
}

void Application::start()
{
    // First we make a Fastcgipp::Manager object, with our request handling
    // class as a template parameter.
    Fastcgipp::Manager<Connection> app;
    // Now just call the object handler function. It will sleep quietly when
    // there are no requests and efficiently manage them when there are
    // many.
    //! [Manager]
    //! [Signals]
    app.setupSignals();
    //! [Signals]
    //! [Listen]
    app.listen();
    //! [Listen]
    //! [Start]
    app.start();
    //! [Start]
    //! [Join]
    app.join();
}


} // namespace core
} // namespace app
