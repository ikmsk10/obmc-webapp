
// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/router.hpp>
#include <core/application.hpp>

namespace app
{
namespace core
{

void Application::registerAllRoutes()
{
    Router::registerUri<GraphqlRouter>("/api/graphql");
}

}
} // namespace app
