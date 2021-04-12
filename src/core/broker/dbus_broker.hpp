// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __DBUSBROKER_H__
#define __DBUSBROKER_H__

#include <core/broker/broker.hpp>
#include <core/entity/entity.hpp>

#include <core/connect/dbusConnect.hpp>
#include <core/entity/query.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace app
{

namespace broker
{

using namespace entity;
using namespace app::query;

class DBusBroker;

using QueryEntityPtr =
    std::shared_ptr<IQuery<IEntity::InstancePtr, sdbusplus::bus::bus>>;
using DBusBrokerPtr = std::shared_ptr<DBusBroker>;

class DBusBroker : public Broker
{
  public:
    DBusBroker(std::chrono::seconds timeout = minutes(0),
               bool watch = false) noexcept :
        Broker(timeout, watch)
    {}
    ~DBusBroker() noexcept override = default;

    // TODO(ik) move to the IConnect interface
    // TODO(ik) make feel free for connect arg
    virtual bool tryProcess(sdbusplus::bus::bus&, sdbusplus::bus::bus&);

    virtual void registerObjectsListener(sdbusplus::bus::bus&);
};

class EntityDbusBroker : public DBusBroker
{
    std::mutex guardMutex;
    entity::EntityPtr entity;
    QueryEntityPtr entityQuery;

  public:
    EntityDbusBroker(entity::EntityPtr entityPointer,
                     QueryEntityPtr queryPointer, bool watch = true,
                     std::chrono::seconds timeout = minutes(0)) noexcept :
        DBusBroker(timeout, watch),
        entity(entityPointer), entityQuery(queryPointer)
    {}
    ~EntityDbusBroker() noexcept override = default;

    bool tryProcess(sdbusplus::bus::bus&, sdbusplus::bus::bus&) override;

    void registerObjectsListener(sdbusplus::bus::bus&) override;
};

class DBusBrokerManager : public IBrokerManager
{
    std::vector<std::thread> threads;
    size_t threadsBrokersTaskCount;
    // const size_t threadsWatchersTaskCount;
    std::atomic_bool active;

    static constexpr size_t defaultBrokerThreadCount = 10;
    static constexpr size_t objectsWatchersTheradCount = 1;
  public:
    DBusBrokerManager(const DBusBrokerManager&) = delete;
    DBusBrokerManager& operator=(const DBusBrokerManager&) = delete;
    DBusBrokerManager(DBusBrokerManager&&) = delete;
    DBusBrokerManager& operator=(DBusBrokerManager&&) = delete;

    explicit DBusBrokerManager(
        size_t brokersTaskCount = defaultBrokerThreadCount) :
        threadsBrokersTaskCount(brokersTaskCount),
        active(false)
    {
      LOG_DEBUG << "Total brokers thread count is " << threadsBrokersTaskCount;
      objectObserverConnect = createDbusConnection();
    }
    ~DBusBrokerManager() noexcept override
    {
    }

    void start() override;
    void terminate() override;
    /**
     * @brief
     *
     */
    void bind(DBusBrokerPtr);

  protected:
    void doCaptureDbus();
    void doManageringObjects();

    connect::DBusConnectUni createDbusConnection();
    bool hasReadyTask() const;
    connect::DBusConnectUni objectObserverConnect;
  private:
    std::vector<DBusBrokerPtr> brokers;
};

} // namespace broker

} // namespace app

#endif // __DBUSBROKER_H__
