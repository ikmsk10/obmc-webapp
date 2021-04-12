// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#include <core/entity/dbus_query.hpp>
#include <core/exceptions.hpp>

namespace app
{
namespace query
{
namespace dbus
{
using namespace app::core::exceptions;

EntityManager::EntityBuilderPtr DBusQueryBuilder::complete()
{
    return std::move(this->entityBuilder);
}

std::size_t FindObjectDBusQuery::DBusObjectEndpoint::getHash() const
{
    std::size_t hashPath = std::hash<std::string>{}(path);
    std::size_t hashService =
        std::hash<std::string>{}(service.has_value() ? *service : "");
    std::size_t hashDepth = std::hash<int32_t>{}(depth);
    std::size_t hashInterfaces;

    for (auto& interface : interfaces)
    {
        hashInterfaces ^= std::hash<std::string>{}(interface);
    }

    return (hashPath ^ (hashInterfaces << 1) ^ (hashDepth << 2) ^
            (hashService << 3));
}

std::vector<IEntity::InstancePtr>
    FindObjectDBusQuery::process(sdbusplus::bus::bus& connect)
{
    using DBusSubTreeOut =
        std::vector<std::pair<std::string, DBusServiceInterfaces>>;
    std::vector<DBusInstancePtr> dbusInstances;

    auto mapperCall = connect.new_method_call(
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    mapperCall.append(getQueryCriteria().path);
    mapperCall.append(getQueryCriteria().depth);
    mapperCall.append(getQueryCriteria().interfaces);

    auto mapperResponseMsg = connect.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        LOG_ERROR << "Error of mapper call";
        throw ObmcAppException("ERROR of mapper call");
    }

    DBusSubTreeOut mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    LOG_DEBUG << "DBus Objects read sucess.";
    for (auto& [objectPath, serviceInfoList] : mapperResponse)
    {
        for (auto& [serviceName, interfaces] : serviceInfoList)
        {
            if (getQueryCriteria().service.has_value() &&
                getQueryCriteria().service.value() != serviceName)
            {
                LOG_DEBUG << "Skip service " << serviceName
                          << " because it was not specified";
                continue;
            }
            LOG_DEBUG << "Emplace new entity instance.";
            auto instance = this->createInstance(connect, serviceName,
                                                 objectPath, interfaces);
            dbusInstances.push_back(instance);
        }
    }

    LOG_DEBUG << "Process Found DBus Object query is sucess. Count instance: "
              << dbusInstances.size();
    std::vector<IEntity::InstancePtr> result(dbusInstances.begin(),
                                             dbusInstances.end());
    return result;
}

void FindObjectDBusQuery::registerObjectCreationObserver(
    sdbusplus::bus::bus& connection)
{
    using namespace sdbusplus::bus::match;

    auto self = shared_from_this();
    auto handler = [entity = self](sdbusplus::message::message& message) {
        DBusInterfacesMap interfacesAdded;
        sdbusplus::message::object_path objectPath;

        try
        {
            message.read(objectPath, interfacesAdded);
        }
        catch (sdbusplus::exception_t&)
        {
            LOG_ERROR << "Can't read added interfaces signal";
            return;
        }

        const std::string& objectPathStr = objectPath.str;

        LOG_DEBUG << "INTERFACES ADDED";
        LOG_DEBUG << "Sender=" << message.get_sender();
        LOG_DEBUG << "Cookie=" << message.get_cookie();
        LOG_DEBUG << "Path=" << objectPathStr;
        LOG_DEBUG << "Member=" << message.get_member();
        LOG_DEBUG << "Sender" << message.get_sender();
        LOG_DEBUG << "Signature=" << message.get_signature();
    };

    match observer(connection,
                   rules::type::signal() + rules::member("InterfacesAdded") +
                       rules::sender("xyz.openbmc_project.ObjectMapper") +
                       rules::argNpath(0, getQueryCriteria().path + "/"),
                   std::move(handler));

    LOG_DEBUG << "registerObjectCreationObserver";
    addObserver(std::forward<match>(observer));
}

void FindObjectDBusQuery::registerObjectRemovingObserver(
    sdbusplus::bus::bus& connection)
{
    using namespace sdbusplus::bus::match;
    std::string sender;
    if (this->getQueryCriteria().service.has_value())
    {
        sender += rules::sender(this->getQueryCriteria().service.value());
    }

    auto handler = [](sdbusplus::message::message& message) {
        sdbusplus::message::object_path objectPath;
        std::vector<InterfaceName> removedInterfaces;

        try
        {
            message.read(objectPath, removedInterfaces);
        }
        catch (sdbusplus::exception_t&)
        {
            LOG_ERROR << "Can't read removed interfaces signal";
            return;
        }

        const std::string& objectPathStr = objectPath.str;

        LOG_DEBUG << "INTERFACES REMOVED";
        LOG_DEBUG << "Sender=" << message.get_sender();
        LOG_DEBUG << "Cookie=" << message.get_cookie();
        LOG_DEBUG << "Path=" << objectPathStr;
        LOG_DEBUG << "Member=" << message.get_member();
        LOG_DEBUG << "Sender" << message.get_sender();
        LOG_DEBUG << "Signature=" << message.get_signature();
        LOG_DEBUG << "UniqueName=" << message.get_bus().get_unique_name();
    };

    LOG_DEBUG << "registerObjectRemovingObserver";
    match observer(connection,
                   rules::type::signal() + rules::member("InterfacesRemoved") +
                       rules::sender("xyz.openbmc_project.ObjectMapper") +
                       rules::argNpath(0, getQueryCriteria().path + "/"),
                   handler);
    addObserver(std::forward<match>(observer));
}

EntityDBusQueryConstWeakPtr FindObjectDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

std::vector<IEntity::InstancePtr>
    IntrospectServiceDBusQuery::process(sdbusplus::bus::bus& connect)
{
    using ObjectValueTree =
        std::map<sdbusplus::message::object_path, DBusInterfacesMap>;
    std::vector<DBusInstancePtr> dbusInstances;
    ObjectValueTree interfacesResponse;

    auto mapperCall = connect.new_method_call(
        serviceName.c_str(), "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");

    auto mapperResponseMsg = connect.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        LOG_ERROR << "Error of mapper call";
        throw ObmcAppException("ERROR of mapper call");
    }

    mapperResponseMsg.read(interfacesResponse);

    for (auto& [objectPath, interfaces] : interfacesResponse)
    {
        auto instance = std::make_shared<DBusInstance>(
            serviceName, objectPath.str, getSearchPropertiesMap(),
            getWeakPtr());

        for (auto& [interfaceName, propertiesMap] : interfaces)
        {
            instance->fillMembers(interfaceName, propertiesMap);
        }
        this->supplementByStaticFields(instance);
        dbusInstances.push_back(instance);
    }

    LOG_DEBUG << "Process Found DBus Object query is sucess.";

    std::vector<IEntity::InstancePtr> result(dbusInstances.begin(),
                                             dbusInstances.end());
    return std::forward<std::vector<IEntity::InstancePtr>>(result);
}

void IntrospectServiceDBusQuery::registerObjectCreationObserver(sdbusplus::bus::bus&)
{

}

void IntrospectServiceDBusQuery::registerObjectRemovingObserver(sdbusplus::bus::bus&)
{

}

EntityDBusQueryConstWeakPtr IntrospectServiceDBusQuery::getWeakPtr() const
{
    return weak_from_this();
}

template <class TInstance>
DBusInstancePtr DBusQuery<TInstance>::createInstance(
    sdbusplus::bus::bus& connect, const ServiceName& serviceName,
    const ObjectPath& objectPath, const std::vector<InterfaceName>& interfaces)
{
    auto entityInstance = std::make_shared<DBusInstance>(
        serviceName, objectPath, getSearchPropertiesMap(), getWeakPtr());

    LOG_DEBUG << "ObjectPath='" << objectPath << "', Service='" << serviceName
              << "'";
    for (auto& interface : interfaces)
    {
        auto properties = entityInstance->queryProperties(connect, interface);
        entityInstance->fillMembers(interface, properties);
    }

    this->supplementByStaticFields(entityInstance);

    return std::forward<DBusInstancePtr>(entityInstance);
}

template <class TInstance>
void DBusQuery<TInstance>::addObserver(sdbusplus::bus::match::match&& observer)
{
    observers.push_back(std::move(observer));
}

const std::vector<DBusInstancePtr> DBusInstance::getComplexInstances() const
{
    std::vector<DBusInstancePtr> childs;
    for (auto [_, instance] : complexInstances)
    {
        childs.push_back(instance);
    }
    return std::forward<const std::vector<DBusInstancePtr>>(childs);
}

void DBusInstance::fillMembers(
    const InterfaceName& interfaceName,
    const std::map<PropertyName, DbusVariantType>& properties)
{
    auto propertyMemberDict = getPropertyMemberDict(interfaceName);
    if (propertyMemberDict.empty())
    {
        LOG_DEBUG << "Properties of interface not provided: " << interfaceName;
        return;
    }

    for (auto& [propertyName, memberName] : propertyMemberDict)
    {
        auto findProperty = properties.find(propertyName);
        if (findProperty == properties.end())
        {
            continue;
        }
        auto formattedValue = dbusQuery.lock()->processFormatters(
            propertyName, findProperty->second);

        this->resolveDBusVariant(memberName, formattedValue);
    }
}

const IEntity::IEntityMember::InstancePtr&
    DBusInstance::getField(const IEntity::EntityMemberPtr& entityMember) const
{
    return getField(entityMember->getName());
}

const IEntity::IEntityMember::InstancePtr&
    DBusInstance::getField(const MemberName& entityMemberName) const
{
    auto findInstanceIt = memberInstances.find(entityMemberName);
    if (findInstanceIt == memberInstances.end())
    {
        // LOG_DEBUG << "The member '" << entityMemberName << "' of object path='"
        //           << this->objectPath << "' not found";
        return instanceNotFound();
    }

    return findInstanceIt->second;
}

bool DBusInstance::hasField(const MemberName& memberName) const
{
    return memberInstances.find(memberName) != memberInstances.end();
}

const DBusPropertiesMap
    DBusInstance::queryProperties(sdbusplus::bus::bus& connect,
                                  const InterfaceName& interface)
{
    std::map<PropertyName, DbusVariantType> properties;

    LOG_DEBUG << "Create DBUs 'GetAll' properties call. Service='"
              << serviceName << "', ObjectPath='" << objectPath
              << "', Interface='" << interface << "'";

    sdbusplus::message::message getProperties = connect.new_method_call(
        this->serviceName.c_str(), this->objectPath.c_str(),
        "org.freedesktop.DBus.Properties", "GetAll");
    getProperties.append(interface);

    try
    {
        sdbusplus::message::message response = connect.call(getProperties);
        response.read(properties);

        LOG_DEBUG << "DBus Properties read SUCCESS. Count properties: "
                  << properties.size();
    }
    catch (const std::exception& e)
    {
        LOG_CRITICAL << "Failed to GetAll properties. PATH=" << objectPath
                     << ", INTF=" << interface << ", WHAT=" << e.what();
    }

    return std::forward<const DBusPropertiesMap>(properties);
}

void DBusInstance::bindListeners(sdbusplus::bus::bus& connection, const EntityPtr& entity)
{
    using namespace sdbusplus::bus::match;

    auto self = shared_from_this();

    for (auto [interface, _] : targetProperties)
    {
        match matcher(
            connection,
            rules::propertiesChanged(this->getObjectPath(), interface),
            [self, entity](sdbusplus::message::message& message) {
                std::map<PropertyName, DbusVariantType> changedValues;
                InterfaceName interfaceName;
                message.read(interfaceName, changedValues);

                self->fillMembers(interfaceName, changedValues);
            });
        LOG_DEBUG << "Registried watcher for Object=" << this->getObjectPath()
                  << ", Interface=" << interface;
        listeners.push_back(std::forward<match>(matcher));
    }
}

const ObjectPath& DBusInstance::getObjectPath() const
{
    return objectPath;
}

const InterfaceName& DBusInstance::getService() const
{
    return serviceName;
}

void DBusInstance::supplement(
    const MemberName& member,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    auto memberInstance = std::make_shared<DBusMemberInstance>(value);
    if (!memberInstances.emplace(member, std::move(memberInstance)).second)
    {
        throw std::logic_error("The requested member '" + member +
                               "' already registried.");
    }
}

void DBusInstance::supplementOrUpdate(
    const MemberName& memberName,
    const IEntity::IEntityMember::IInstance::FieldType& value)
{
    if (hasField(memberName))
    {
        getField(memberName)->setValue(value);
        return;
    }

    supplement(memberName, value);
}

bool DBusInstance::checkCondition(const IEntity::ConditionPtr condition) const
{
    LOG_DEBUG << "Checking condition";
    return !condition || condition->check(*this);
}

std::size_t DBusInstance::getHash() const
{
    std::size_t hashPath = std::hash<std::string>{}(objectPath);
    std::size_t hashServiceName = std::hash<std::string>{}(serviceName);

    return (hashPath ^ (hashServiceName << 1));
}

template <typename TProperty>
void DBusInstance::captureComplexDBusProperty(const MemberName& memberName,
                                              const TProperty& property)

{
    LOG_DEBUG << "Complex Primitive capture, member name=" << memberName;
    for (auto& value : property)
    {
        DBusPropertiesMap properties{
            {memberName, value},
        };
        auto complexInstance = std::make_shared<DBusInstance>(
            serviceName, objectPath, targetProperties, dbusQuery);
        complexInstance->fillMembers(memberName, properties);
        this->complexInstances.insert_or_assign(complexInstance->getHash(),
                                              complexInstance);
    }
    return;
}

const IEntity::IEntityMember::InstancePtr&
    DBusInstance::instanceNotFound() const
{
    static IEntity::IEntityMember::InstancePtr notAvailable =
        std::make_shared<Entity::EntityMember::StaticInstance>(
            std::string(Entity::EntityMember::fieldValueNotAvailable));

    return notAvailable;
}

void DBusInstance::captureDBusAssociations(
    const InterfaceName& interfaceName,
    const DBusAssociationsType& associations)
{
    using namespace app::entity::obmc::definitions::supplement_providers;

    LOG_DEBUG << "Complex Association values process, member name="
              << interfaceName;

    this->complexInstances.clear();
    // Since the complex association is a disclose of shadow one DBus Property,
    // we should clear outdated instances with the same specified interface
    for (auto& [source, destination, destObjectPath] : associations)
    {
        LOG_DEBUG << "Loop association: Source=" << source
                  << ", Destination=" << destination
                  << ", ObjectPath=" << destObjectPath;

        auto childInstance = std::make_shared<DBusInstance>(
            serviceName, destObjectPath, targetProperties, dbusQuery);
        DBusPropertiesMap properties{
            {relations::fieldSource, source},
            {relations::fieldDestination, destination},
            {relations::fieldEndpoint, destObjectPath},
        };
        // Don't care about outdated instances. The 'fillMemeber' method now be
        // able to update stored instances.
        childInstance->fillMembers(interfaceName, properties);

        this->complexInstances.insert_or_assign(childInstance->getHash(),
                                              childInstance);
    }
}

void DBusInstance::resolveDBusVariant(const MemberName& memberName,
                                      const DbusVariantType& dbusVariant)
{
    auto self = shared_from_this();
    auto visitCallback = [instance = self, memberName](auto&& value) {
        using TProperty = std::decay_t<decltype(value)>;

        if constexpr (std::is_constructible_v<DBusMemberInstance::FieldType,
                                              TProperty>)
        {
            instance->supplementOrUpdate(memberName,
                                         DBusMemberInstance::FieldType(value));
            return;
        }
        else if constexpr (std::is_same_v<std::vector<std::string>,
                                          TProperty> ||
                           std::is_same_v<std::vector<double>, TProperty>)
        {
            instance->captureComplexDBusProperty(memberName, value);
            return;
        }
        else if constexpr (std::is_same_v<DBusAssociationsType, TProperty>)
        {
            instance->captureDBusAssociations(memberName, value);
            return;
        }

        throw std::invalid_argument(
            "Try to retrieve the dbus property of an unsupported type");
    };
    std::visit(std::move(visitCallback), dbusVariant);
}

const std::map<std::size_t, IEntity::InstancePtr>
    DBusInstance::getComplex() const
{
    std::map<std::size_t, IEntity::InstancePtr> result(complexInstances.begin(),
                                                       complexInstances.end());
    return std::forward<std::map<std::size_t, IEntity::InstancePtr>>(result);
}

bool DBusInstance::isComplex() const
{
    return complexInstances.empty();
}

void DBusInstance::initDefaultFieldsValue()
{
    auto& defaultFields = dbusQuery.lock()->getDefaultFieldsValue();

    for(auto& [memberName, memberValue]: defaultFields)
    {
        this->supplementOrUpdate(memberName, memberValue);
    }
}

const DBusPropertyMemberDict
    DBusInstance::getPropertyMemberDict(const InterfaceName& interface) const
{
    auto findInterface = this->targetProperties.find(interface);
    if (findInterface == this->targetProperties.end())
    {
        LOG_DEBUG << "Interface '" << interface << "' missmatch. Skipping";
        return DBusPropertyMemberDict();
    }

    return std::move(findInterface->second);
}

const IEntity::IEntityMember::IInstance::FieldType&
    DBusMemberInstance::getValue() const noexcept
{
    return value;
}

const std::string& DBusMemberInstance::getStringValue() const
{
    return std::get<std::string>(value);
}

int DBusMemberInstance::getIntValue() const
{
    return std::get<int>(value);
}

double DBusMemberInstance::getFloatValue() const
{
    return std::get<double>(value);
}

bool DBusMemberInstance::getBoolValue() const
{
    return std::get<bool>(value);
}

void DBusMemberInstance::setValue(const FieldType& value)
{
    this->value = value;
}

template <class TInstance>
const DbusVariantType
    DBusQuery<TInstance>::processFormatters(const PropertyName& property,
                                            const DbusVariantType& value) const

{
    if (getFormatters().empty())
    {
        LOG_DEBUG << "No formatters";
        return std::move(value);
    }

    auto findFormattersIt = getFormatters().find(property);
    if (getFormatters().end() == findFormattersIt)
    {
        return std::move(value);
    }

    LOG_DEBUG << "Found formatter for Property=" << property;
    DbusVariantType formattedValue(value);

    for (auto& formatter : findFormattersIt->second)
    {
        formattedValue = std::invoke(formatter, property, formattedValue);
    }

    return formattedValue;
}

template <class TInstance>
const DBusQuery<TInstance>::FieldsFormattingMap&
    DBusQuery<TInstance>::getFormatters() const
{
    static const FieldsFormattingMap noFormattes;
    return noFormattes;
}

template class DBusQuery<IEntity::InstancePtr>;

} // namespace dbus
} // namespace query
} // namespace app
