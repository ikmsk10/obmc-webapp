// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __SYSTEM_QUERIES_H__
#define __SYSTEM_QUERIES_H__

#include <logger/logger.hpp>
#include <core/exceptions.hpp>

#include <core/entity/dbus_query.hpp>
#include <core/entity/entity.hpp>

#include <core/helpers/compares.hpp>

#include <definitions.hpp>

#include <status_provider.hpp>
#include <version_provider.hpp>

namespace app
{
namespace query
{
namespace obmc
{

using namespace app::entity;
using namespace app::query;
using namespace app::query::dbus;

using namespace app::entity::obmc::definitions;

using namespace std::placeholders;

using VariantType = dbus::DbusVariantType;
using FieldSet = std::vector<std::string>;

namespace general
{
namespace assets
{

constexpr const char* assetInterface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

constexpr const char* propertyManufacturer = "Manufacturer";
constexpr const char* propertyModel = "Model";
constexpr const char* propertyPartNumber = "PartNumber";
constexpr const char* propertySerialNumber = "SerialNumber";
} // namespace assets
} // namespace general

class Server final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* chassisInterface =
        "xyz.openbmc_project.Inventory.Item.Chassis";
    static constexpr const char* namePropertyName = "Name";
    static constexpr const char* namePropertyType = "Type";

  public:
    Server() : dbus::FindObjectDBusQuery()
    {}
    ~Server() noexcept override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                chassisInterface,
                {
                    {
                        namePropertyName,
                        app::entity::obmc::definitions::fieldName,
                    },
                    {
                        namePropertyType,
                        app::entity::obmc::definitions::fieldType,
                    },
                },
            },
            {
                general::assets::assetInterface,
                {
                    {
                        general::assets::propertyManufacturer,
                        app::entity::obmc::definitions::fieldManufacturer,
                    },
                    {
                        general::assets::propertyModel,
                        app::entity::obmc::definitions::fieldModel,
                    },
                    {
                        general::assets::propertySerialNumber,
                        app::entity::obmc::definitions::fieldSerialNumber,
                    },
                    {
                        general::assets::propertyPartNumber,
                        app::entity::obmc::definitions::fieldPartNumber,
                    },
                },
            },
        };

        return dictionary;
    }

    static void linkVersions(const IEntity::InstancePtr& supplementing,
                           const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc;
        using namespace definitions::supplement_providers::version;

        static std::map<Version::VersionPurpose, MemberName>
            purposeToMemberName{
                {Version::VersionPurpose::BMC,
                 definitions::version::fieldVersionBmc},
                {Version::VersionPurpose::BIOS,
                 definitions::version::fieldVersionBios},
            };
        try
        {
            auto purpose = Version::getPurpose(supplementing);
            if (Version::VersionPurpose::Unknown == purpose)
            {
                LOG_ERROR << "Accepted unknown version purpose. Skipping";
                return;
            }

            auto findMemberNameIt = purposeToMemberName.find(purpose);
            if (findMemberNameIt == purposeToMemberName.end())
            {
                throw app::core::exceptions::InvalidType("Version Purpose");
            }

            auto versionValue =
                supplementing->getField(fieldVersion)->getValue();
            target->supplementOrUpdate(findMemberNameIt->second, versionValue);
        }
        catch (std::bad_variant_access& ex)
        {
            LOG_ERROR << "Can't supplement the 'Version' field of 'Server' "
                         "Entity. Reason: "
                      << ex.what();
        }
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/chassis/",
            {
                chassisInterface,
            },
            1U,
        };

        return criteria;
    }
};

class Chassis final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* yadroInterface = "com.yadro.Platform";

    static constexpr const char* namePropertyType = "ChassisType";
    static constexpr const char* namePropertyPartNumber = "ChassisPartNumber";

    static constexpr const char* chassisTypeOther = "Other";
    static const std::map<std::string, std::string> chassisTypesNames;

  public:
    Chassis() : dbus::FindObjectDBusQuery()
    {}
    ~Chassis() noexcept = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                yadroInterface,
                {
                    {
                        namePropertyType,
                        app::entity::obmc::definitions::fieldType,
                    },
                    {
                        namePropertyPartNumber,
                        app::entity::obmc::definitions::fieldPartNumber,
                    },
                },
            },
            {
                general::assets::assetInterface,
                {
                    {
                        general::assets::propertyManufacturer,
                        app::entity::obmc::definitions::fieldManufacturer,
                    },
                },
            },
        };

        return dictionary;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/board/",
            {
                yadroInterface,
            },
            1U,
        };

        return criteria;
    }

    static const DbusVariantType formatChassisType(const PropertyName&,
                                                   const DbusVariantType& value)
    {
        auto formattedValue = std::visit(
            [](auto&& chassisType) -> const DbusVariantType {
                using TChassisType = std::decay_t<decltype(chassisType)>;

                if constexpr (std::is_same_v<TChassisType, std::string>)
                {
                    auto findTypeIt = chassisTypesNames.find(chassisType);
                    if (findTypeIt == chassisTypesNames.end())
                    {
                        return DbusVariantType(std::string(chassisTypeOther));
                    }

                    return DbusVariantType(findTypeIt->second);
                }

                throw std::invalid_argument(
                    "Invalid value type of ChassisType property");
            },
            value);
        return formattedValue;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        static const FieldsFormattingMap formatters{
            {
                namePropertyType,
                {
                    Chassis::formatChassisType,
                },
            },
        };

        return formatters;
    }
};

class Baseboard final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* boardInterface =
        "xyz.openbmc_project.Inventory.Item.Board";
    static constexpr const char* namePropertyName = "Name";
    static constexpr const char* namePropertyType = "Type";

  public:
    Baseboard() : dbus::FindObjectDBusQuery()
    {}
    ~Baseboard() noexcept override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                boardInterface,
                {
                    {
                        namePropertyName,
                        app::entity::obmc::definitions::fieldName,
                    },
                    {
                        namePropertyType,
                        app::entity::obmc::definitions::fieldType,
                    },
                },
            },
            {
                general::assets::assetInterface,
                {
                    {
                        general::assets::propertyManufacturer,
                        app::entity::obmc::definitions::fieldManufacturer,
                    },
                    {
                        general::assets::propertyModel,
                        app::entity::obmc::definitions::fieldModel,
                    },
                    {
                        general::assets::propertySerialNumber,
                        app::entity::obmc::definitions::fieldSerialNumber,
                    },
                    {
                        general::assets::propertyPartNumber,
                        app::entity::obmc::definitions::fieldPartNumber,
                    },
                },
            },
        };

        return dictionary;
    }

    static std::vector<MemberName>
        relationsSensorsLinkRule(const IEntity::InstancePtr& supplementing,
                       const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc::definitions;
        using namespace app::entity::obmc::definitions::supplement_providers;

        std::visit(
            [](auto&& relations) {
                using TProperty = std::decay_t<decltype(relations)>;

                if constexpr (std::is_same_v<std::string,
                                             TProperty>)
                {
                    for (auto& relation : relations)
                    {
                        LOG_DEBUG << "Relation: " << relation;
                    }
                }
            },
            supplementing->getField(relations::fieldEndpoint)->getValue());

        return {
            metaRelation,
        };
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/inventory/system/board/",
            {
                boardInterface,
            },
            1U,
        };

        return criteria;
    }
};

class Sensors final : public dbus::FindObjectDBusQuery
{
    static constexpr const char* sensorThresholdWarningInterface =
        "xyz.openbmc_project.Sensor.Threshold.Warning";
    static constexpr const char* sensorThresholdCriticalInterface =
        "xyz.openbmc_project.Sensor.Threshold.Critical";
    static constexpr const char* sensorValueInterface =
        "xyz.openbmc_project.Sensor.Value";

    static constexpr const char* namePropertyCriticalLow = "CriticalLow";
    static constexpr const char* namePropertyCriticalHigh = "CriticalHigh";
    static constexpr const char* namePropertyWarningLow = "WarningLow";
    static constexpr const char* namePropertyWarningHigh = "WarningHigh";
    static constexpr const char* namePropertyValue = "Value";
    static constexpr const char* namePropertyUnit = "Unit";

    static constexpr const char* defaultUnitLabel = "Unit";

    static const std::map<std::string, std::string> sensorUnitsMap;

  public:
    Sensors() : dbus::FindObjectDBusQuery()
    {}
    ~Sensors() noexcept override = default;

    const dbus::DBusPropertyEndpointMap& getSearchPropertiesMap() const override
    {
        static const dbus::DBusPropertyEndpointMap dictionary{
            {
                sensorValueInterface,
                {
                    {
                        namePropertyValue,
                        app::entity::obmc::definitions::sensors::fieldValue,
                    },
                    {
                        namePropertyUnit,
                        app::entity::obmc::definitions::sensors::fieldUnit,
                    },
                },
            },
            {
                sensorThresholdWarningInterface,
                {
                    {
                        namePropertyWarningLow,
                        app::entity::obmc::definitions::sensors::
                            fieldLowWarning,
                    },
                    {
                        namePropertyWarningHigh,
                        app::entity::obmc::definitions::sensors::
                            fieldHighWarning,
                    },
                },
            },
            {
                sensorThresholdCriticalInterface,
                {
                    {
                        namePropertyCriticalHigh,
                        app::entity::obmc::definitions::sensors::
                            fieldHightCritical,
                    },
                    {
                        namePropertyCriticalLow,
                        app::entity::obmc::definitions::sensors::
                            fieldLowCritical,
                    },
                },
            },
        };

        return dictionary;
    }

    static void linkStatus(const IEntity::InstancePtr& supplementing,
                           const IEntity::InstancePtr& target)
    {
        using namespace app::entity::obmc;
        using namespace definitions::supplement_providers;
        using namespace status;

        try
        {
            auto& targetObjectPath = target->getField(metaObjectPath)->getStringValue();
            auto& causer =
                supplementing->getField(fieldObjectCauthPath)->getStringValue();

            auto candidate =
                supplementing->getField(fieldStatus)->getStringValue();
            auto current = target->getField(fieldStatus)->getStringValue();

            if (targetObjectPath != causer)
            {
                return;
            }

            LOG_DEBUG << "Supplementing object=" << targetObjectPath
                      << " field Status=" << candidate
                      << ". Current Value=" << current;

            target->getField(status::fieldStatus)
                ->setValue(Status::getHigherStatus(current, candidate));
        }
        catch (std::bad_variant_access& ex)
        {
            LOG_ERROR << "Can't supplement the 'Status' field of 'Sensor' "
                         "Entity. Reason: "
                      << ex.what();
        }
    }

    void supplementByStaticFields(DBusInstancePtr& instance) override
    {
        this->setSensorName(instance);
    }

    const DefaultFieldsValueDict& getDefaultFieldsValue() const
    {
        using namespace app::entity::obmc::definitions::supplement_providers;
        static const DefaultFieldsValueDict defaultStatusOk{
            {status::fieldStatus, std::string(Status::statusOK)},
        };
        return defaultStatusOk;
    }

  protected:
    const DBusObjectEndpoint& getQueryCriteria() const override
    {
        static const DBusObjectEndpoint criteria{
            "/xyz/openbmc_project/sensors",
            {
                sensorValueInterface,
            },
            noDepth,
        };

        return criteria;
    }

    void setSensorName(DBusInstancePtr& instance)
    {
        auto& objectPath = instance->getObjectPath();
        instance->supplement(
            app::entity::obmc::definitions::sensors::fieldName,
            app::helpers::utils::getNameFromLastSegmentObjectPath(objectPath));
    }

    static const DbusVariantType formatSensorUnit(const PropertyName&,
                                                  const DbusVariantType& value)
    {
        auto formattedValue = std::visit(
            [](auto&& chassisType) -> const DbusVariantType {
                using TChassisType = std::decay_t<decltype(chassisType)>;

                if constexpr (std::is_same_v<TChassisType, std::string>)
                {
                    auto findTypeIt = sensorUnitsMap.find(chassisType);
                    if (findTypeIt == sensorUnitsMap.end())
                    {
                        return DbusVariantType(std::string(defaultUnitLabel));
                    }

                    return DbusVariantType(findTypeIt->second);
                }

                throw std::invalid_argument(
                    "Invalid value type of Sensor Unit property");
            },
            value);
        return formattedValue;
    }

    const FieldsFormattingMap& getFormatters() const override
    {
        static const FieldsFormattingMap formatters{
            {
                namePropertyUnit,
                {
                    Sensors::formatSensorUnit,
                },
            },
        };

        return formatters;
    }
};

} // namespace obmc
} // namespace query
} // namespace app

#endif // __SYSTEM_QUERIES_H__
