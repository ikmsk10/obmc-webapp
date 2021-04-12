// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <config.h>

#include <core/broker/dbus_broker.hpp>
#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>
#include <sdbusplus/bus/match.hpp>

#include <thread>
#include <vector>

namespace app
{
namespace broker
{
bool DBusBroker::tryProcess(sdbusplus::bus::bus&, sdbusplus::bus::bus&)
{
    if (!isTimeout())
    {
        LOG_DEBUG << "Attempt to get to process the task which "
                     "not timed yet.";
        return false;
    }

    return true;
}

bool EntityDbusBroker::tryProcess(sdbusplus::bus::bus& queryConnect,
                                  sdbusplus::bus::bus& watcherConnect)
{
    std::unique_lock<std::mutex> lock(guardMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        LOG_DEBUG << "Attempt to get to process a broker's task which "
                     "already runs.";
        return false;
    }

    auto result = DBusBroker::tryProcess(queryConnect, watcherConnect);
    if (!result)
    {
        return false;
    }

    if (!this->entityQuery)
    {
        throw app::core::exceptions::ObmcAppException(
            "Query not registried! Entity=" + entity->getName());
    }
    LOG_DEBUG << "Accept broker task of Entity '" << entity->getName() << "'";

    auto instances = this->entityQuery->process(queryConnect);

    // Register watchers if the broker is configured to watch of DBus signals.
    if (this->isWatch())
    {
        for (auto& instance : instances)
        {
            auto dbusInstance =
                std::dynamic_pointer_cast<dbus::DBusInstance>(instance);
            if (!dbusInstance)
            {
                throw std::logic_error("At the DBusBroker the entity query "
                                       "accepted instance which is "
                                       "not DBusInstance.");
            }
            dbusInstance->bindListeners(watcherConnect, this->entity);
        }
    }
    this->entity->setInstances(instances);
    this->setExecutionTime();

    LOG_DEBUG << "Process query is sucess. Entity '" << entity->getName()
              << "'";
    return true;
}

void EntityDbusBroker::registerObjectsListener(sdbusplus::bus::bus& connect)
{
    auto dbusQuery =
        std::dynamic_pointer_cast<query::dbus::EntityDBusQuery>(entityQuery);

    if (!dbusQuery)
    {
        throw std::logic_error(
            "At the DBusBroker the entity query is not EntityDBusQuery.");
    }

    dbusQuery->registerObjectCreationObserver(connect);
    dbusQuery->registerObjectRemovingObserver(connect);
}

void DBusBrokerManager::start()
{
    static const std::vector<std::pair<size_t, std::function<void()>>>
        countTaskThreadsDict{
            {threadsBrokersTaskCount,
             std::bind(&DBusBrokerManager::doCaptureDbus, this)},
            {objectsWatchersTheradCount,
             std::bind(&DBusBrokerManager::doManageringObjects, this)},
        };

    active = true;
    size_t totalTasks = 0;
    for (auto& [count, handler] : countTaskThreadsDict)
    {
        totalTasks += count;
        for (size_t threadNumber = 0; threadNumber < count; threadNumber++)
        {
            LOG_DEBUG << "init broker task #" << threadNumber;
            threads.push_back(std::thread(handler));
        }
    }
    LOG_INFO << "Total " << totalTasks << " DBus brokers tasks started";
}

void DBusBrokerManager::terminate()
{
    active = false;
    LOG_INFO << "Request to terminate DBus brokers";

    for (auto& thread : threads)
    {
        thread.join();
    }
}

void DBusBrokerManager::bind(DBusBrokerPtr broker)
{
    auto& connection = this->objectObserverConnect->getConnect();
    if (broker->isWatch())
    {
        broker->registerObjectsListener(*connection);
    }
    this->brokers.push_back(broker);
}

void DBusBrokerManager::doCaptureDbus()
{
    auto dbusConnect = createDbusConnection();
    auto dbusMatchConnect = createDbusConnection();

    while (active)
    {
        auto& connection = dbusConnect->getConnect();

        for (const auto& broker : brokers)
        {
            if (broker->isTimeout())
            {
                LOG_DEBUG << "Try process broker task: #"
                          << std::this_thread::get_id();
                if (!broker->tryProcess(*connection,
                                        *dbusMatchConnect->getConnect()))
                {
                    LOG_WARNING << "Cant process broker task";
                }
            }
        }

        // Process watcher while haven't ready tasks
        dbusMatchConnect->getConnect()->wait(5s);
        dbusMatchConnect->getConnect()->process_discard();
    }
    LOG_INFO << "Terminate broker ID #" << std::this_thread::get_id();
}

void DBusBrokerManager::doManageringObjects()
{
    while(active)
    {
        auto& connection = objectObserverConnect->getConnect();
        connection->wait(5s);
        connection->process_discard();
    }
    LOG_INFO << "Terminate object watcher ID #" << std::this_thread::get_id();
}

connect::DBusConnectUni DBusBrokerManager::createDbusConnection()
{
#ifdef BMC_DBUS_CONNECT_SYSTEM
    connect::DBusConnectUni dbusConnect =
        std::make_unique<app::connect::SystemDBusConnect>();
#elif defined(BMC_DBUS_CONNECT_REMOTE)
    connect::DBusConnectUni dbusConnect =
        std::make_unique<app::connect::RemoteHostDbusConnect>(
            BMC_DBUS_REMOTE_HOST);
#else
#error "Unknown DBus connection type"
#endif
    dbusConnect->connect();

    return std::forward<connect::DBusConnectUni>(dbusConnect);
}

bool DBusBrokerManager::hasReadyTask() const
{
    for (const auto& broker : brokers)
    {
        if (broker->isTimeout())
        {
           return true;
        }
    }
    return false;
}

} // namespace broker
} // namespace app
