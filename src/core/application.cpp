// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/application.hpp>
#include <core/connection.hpp>
#include <core/route/handlers/graphql_handler.hpp>

#include <core/entity/entity.hpp>
#include <core/entity/dbus_query.hpp>

#include <service/configuration.hpp>

#include <definitions.hpp>

#include <system_queries.hpp>
#include <status_provider.hpp>
#include <relation_provider.hpp>
#include <version_provider.hpp>

#include <csignal>

namespace app
{
namespace core
{

void Application::configure()
{
    LOG_DEBUG << "configure";
    /* TODO set Threads/Features/APIs */
    registerAllRoutes();

    // TODO include IProtocol abstraction which incapsulate own specific handlers.
    // Need to remove a static class/methods.
    route::handlers::VisitorFactory::registerGqlVisitors();

    this->initEntityMap();
    this->initBrokers();

    // register handlers
    std::signal(SIGINT, &Application::handleSignals);
}

void Application::start()
{
    /* First we make a Fastcgipp::Manager object, with our request handling
     * class as a template parameter.
     */
    Fastcgipp::Manager<Connection> app;
    /* Now just call the object handler function. It will sleep quietly when
     * there are no requests and efficiently manage them when there are many.
     */
    app.setupSignals();
    app.listen();
    app.start();
    app.join();
}

void Application::terminate()
{
    dbusBrokerManager.terminate();
}

void Application::initEntityMap()
{
    using namespace app::entity;
    using namespace app::entity::obmc;
    using namespace app::query;
    using namespace app::query::obmc;
    using namespace definitions::supplement_providers;

    using namespace std::literals;

    constexpr bool observeDBusSignals = true;

    /* Define GLOBAL STATUS supplement provider */
    entityManager
        .buildSupplementProvider(
            status::providerStatus)
        ->addMembers({
            status::fieldStatus,
            status::fieldObjectCauthPath,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Status>()
        .complete();

    /* Define GLOBAL VERSION supplement provider */
    entityManager
        .buildSupplementProvider(
            definitions::supplement_providers::version::providerVersion)
        ->addMembers({
            definitions::supplement_providers::version::fieldPurpose,
            definitions::supplement_providers::version::fieldVersion,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Version>()
        .complete();

    /* Define GLOBAL ASSOCIATIONS supplement provider */
    entityManager
        .buildEntity(
            relations::providerRelations)
        ->addMembers({
            relations::fieldEndpoint,
            relations::fieldSource,
            relations::fieldDestination,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Relation>()
        .complete();

      /* Define SENSORS entity */
    entityManager.buildEntity(definitions::entitySensors)
        ->addMembers({
            definitions::sensors::fieldName,
            definitions::sensors::fieldValue,
            definitions::sensors::fieldUnit,
            definitions::sensors::fieldLowWarning,
            definitions::sensors::fieldLowCritical,
            definitions::sensors::fieldHighWarning,
            definitions::sensors::fieldHightCritical,
            status::fieldStatus,
        })
        .linkSupplementProvider(
            status::providerStatus,
            std::bind(&Sensors::linkStatus, _1, _2))
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Sensors>(observeDBusSignals, 5min)
        .complete();

    /* Define SERVER entity */
    entityManager.buildEntity(definitions::entityServer)
        ->addMembers({
            definitions::fieldName,
            definitions::fieldType,
            definitions::fieldManufacturer,
            definitions::fieldModel,
            definitions::fieldPartNumber,
            definitions::fieldSerialNumber,
            definitions::version::fieldVersionBios,
            definitions::version::fieldVersionBmc,
            status::fieldStatus,
        })
        .linkSupplementProvider(
            definitions::supplement_providers::version::providerVersion,
            std::bind(&Server::linkVersions, _1, _2))
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Server>()
        .complete();

    /* Define CHASSIS entity */
    entityManager.buildEntity(definitions::entityChassis)
        ->addMembers({
            definitions::fieldType,
            definitions::fieldPartNumber,
            definitions::fieldManufacturer,
            status::fieldStatus,
        })
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Chassis>()
        .complete();

    /* Define BASEBOARD entity */
    entityManager.buildEntity(definitions::entityBaseboard)
        ->addMembers({
            definitions::fieldName,
            definitions::fieldType,
            definitions::fieldManufacturer,
            definitions::fieldModel,
            definitions::fieldPartNumber,
            definitions::fieldSerialNumber,
            status::fieldStatus,
            relations::fieldEndpoint,
        })
        // .linkSupplementProvider(
        //     status::providerStatus,
        //     std::bind(&Baseboard::statusLinkRule, _1, _2))
        //  .linkSupplementProvider(
        //     relations::providerRelations,
        //     std::bind(&Baseboard::relationsSensorsLinkRule, _1, _2))
        .addQuery<dbus::DBusQueryBuilder>(dbusBrokerManager)
        ->addObject<Baseboard>()
        .complete();
}

void Application::initBrokers()
{
    dbusBrokerManager.start();
    LOG_DEBUG << "Start DBus broker";
}

void Application::handleSignals(int signal)
{
    LOG_DEBUG << "Catch signal: " << strsignal(signal);
    if (SIGINT == signal) {
        LOG_DEBUG << "SIGINT handle";

        application.terminate();
    }
}


} // namespace core
} // namespace app
