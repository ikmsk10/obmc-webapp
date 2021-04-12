// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <logger/logger.hpp>
#include <core/exceptions.hpp>
#include <core/helpers/utils.hpp>

#include <service/session.hpp>
#include <service/configuration.hpp>

#include <map>
#include <string>
#include <functional>
#include <filesystem>
#include <fstream>

namespace app
{
namespace service
{
namespace config
{

using ConfigFilePath = std::string;
using ConfigFileName = std::string;

/** Configuration file storage metadata depending on storage type. */
static const std::map<session::configFileStorageType,
                      std::pair<ConfigFilePath, ConfigFileName>>
    configFilePathDict{
        {session::configFileStorageType::configCurrentBmcSession,
         {"/tmp/bmcweb_metadata", "sessions.json"}},
        /* The configuration for the current bmcweb service session will be
           stored in a 'heap' */
    };

class ConfigFile;

class ConfigFile
{
    using TPopulate = std::function<void(nlohmann::json&)>;

    static constexpr const char* keyRevision = "revision";
    static constexpr const char* keyAuthConfig = "auth_config";
    static constexpr const char* keySystemUUID = "system_uuid";
    static constexpr const char* keyTimeout = "timeout";
    static constexpr const char* keySessions = "sessions";

  public:
    ConfigFile()
    {
        readData();
    }

    ~ConfigFile()
    {
        // Make sure we aren't writing stale sessions
        session::SessionStore::getInstance().applySessionTimeouts();
    }

    void readData()
    {
        for (auto& [storageType, sotrageMetaData] : configFilePathDict)
        {
            readData(storageType, sotrageMetaData.first,
                     sotrageMetaData.second);
        }
    }

    const std::string& getSystemUUID()
    {
        return systemUuid;
    }

  private:
    // TODO(ed) this should really use protobuf, or some other serialization
    // library, but adding another dependency is somewhat outside the scope of
    // this application for the moment
    void readData(const session::configFileStorageType storageType,
                  const ConfigFilePath& configPath,
                  const ConfigFileName& configFileName)
    {
        std::ifstream persistentFile(configPath + "/" + configFileName);
        uint64_t fileRevision = 0;
        if (persistentFile.is_open())
        {
            // call with exceptions disabled
            auto data = nlohmann::json::parse(persistentFile, nullptr, false);
            if (data.is_discarded())
            {
                LOG_ERROR
                    << "Error parsing persistent data in json file.";
            }
            else
            {
                for (const auto& item : data.items())
                {
                    if (item.key() == keyRevision)
                    {
                        fileRevision = 0;

                        const uint64_t* uintPtr =
                            item.value().get_ptr<const uint64_t*>();
                        if (uintPtr == nullptr)
                        {
                            LOG_ERROR << "Failed to read revision flag";
                        }
                        else
                        {
                            fileRevision = *uintPtr;
                        }
                    }
                    else if (item.key() == keySystemUUID)
                    {
                        const std::string* jSystemUuid =
                            item.value().get_ptr<const std::string*>();
                        if (jSystemUuid != nullptr)
                        {
                            systemUuid = *jSystemUuid;
                        }
                    }
                    else if (item.key() == keyAuthConfig)
                    {
                        session::SessionStore::getInstance()
                            .getAuthMethodsConfig()
                            .fromJson(item.value());
                    }
                    else if (item.key() == keySessions)
                    {
                        for (const auto& elem : item.value())
                        {
                            std::shared_ptr<session::UserSession> newSession =
                                session::UserSession::fromJson(elem);

                            if (newSession == nullptr)
                            {
                                LOG_ERROR << "Problem reading session "
                                                    "from persistent store";
                                continue;
                            }
                            newSession->storageType = storageType;
                            LOG_DEBUG
                                << "Restored session: " << newSession->csrfToken
                                << " " << newSession->uniqueId << " "
                                << newSession->sessionToken;
                            session::SessionStore::getInstance().authTokens.emplace(
                                newSession->sessionToken, newSession);
                        }
                    }
                    else if (item.key() == keyTimeout)
                    {
                        const int64_t* jTimeout =
                            item.value().get_ptr<int64_t*>();
                        if (jTimeout == nullptr)
                        {
                            LOG_DEBUG
                                << "Problem reading session timeout value";
                            continue;
                        }
                        std::chrono::seconds sessionTimeoutInseconds(*jTimeout);
                        LOG_DEBUG << "Restored Session Timeout: "
                                         << sessionTimeoutInseconds.count();
                        session::SessionStore::getInstance().updateSessionTimeout(
                            sessionTimeoutInseconds);
                    }
                    else
                    {
                        // Do nothing in the case of extra fields.  We may have
                        // cases where fields are added in the future, and we
                        // want to at least attempt to gracefully support
                        // downgrades in that case, even if we don't officially
                        // support it
                    }
                }
            }
        }

        if (systemUuid.empty())
        {
            throw app::core::exceptions::ObmcAppException(
                "The actual Sysyten UUID is not captured.");
        }
    }

    inline void createConfigPath(const ConfigFilePath& configPath)
    {
        if (std::filesystem::exists(configPath))
        {
            LOG_DEBUG << "The configuration file path already exists.";
            return;
        }

        std::filesystem::create_directories(configPath);
        std::filesystem::permissions(configPath, configPermission);
    }

    void populateRevision(nlohmann::json& config)
    {
        config[keyRevision] = jsonRevision;
    }

    void populateSystemUUID(nlohmann::json& config)
    {
        config[keySystemUUID] = systemUuid;
    }

    void populateAuthConfig(nlohmann::json& config)
    {
        const auto& authMethods =
            session::SessionStore::getInstance().getAuthMethodsConfig();
        nlohmann::json& authConfig = config[keyAuthConfig];
        authConfig =
            nlohmann::json::object({{"XToken", authMethods.xtoken},
                                    {"Cookie", authMethods.cookie},
                                    {"SessionToken", authMethods.sessionToken},
                                    {"BasicAuth", authMethods.basic},
                                    {"TLS", authMethods.tls}});
    }

    void populateTimeout(nlohmann::json& config)
    {
        config[keyTimeout] =
            session::SessionStore::getInstance().getTimeoutInSeconds();
    }

    void populateSessions(nlohmann::json& config,
                          const session::configFileStorageType storageType)
    {
        nlohmann::json& sessions = config[keySessions];
        sessions = nlohmann::json::array();
        for (const auto& p : session::SessionStore::getInstance().authTokens)
        {
            if (p.second->persistence !=
                    session::PersistenceType::SINGLE_REQUEST &&
                p.second->storageType == storageType)
            {
                sessions.push_back({
                    {"unique_id", p.second->uniqueId},
                    {"session_token", p.second->sessionToken},
                    {"username", p.second->username},
                    {"csrf_token", p.second->csrfToken},
                    {"client_ip", p.second->clientIp},
                });
            }
        }
    }

    /** Configuration fields definition depending on storage type. */
    const std::map<session::configFileStorageType, std::vector<TPopulate>>
        configPopulatHandlersDict{
            {
                session::configFileStorageType::configLongTermStorage,
                {
                    std::bind(&ConfigFile::populateRevision, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateAuthConfig, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateSystemUUID, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateTimeout, this,
                              std::placeholders::_1),
                    std::bind(
                        &ConfigFile::populateSessions, this,
                        std::placeholders::_1,
                        session::configFileStorageType::configLongTermStorage),
                },
            },
            {
                session::configFileStorageType::configCurrentBmcSession,
                {
                    std::bind(&ConfigFile::populateRevision, this,
                              std::placeholders::_1),
                    std::bind(&ConfigFile::populateSessions, this,
                              std::placeholders::_1,
                              session::configFileStorageType::
                                  configCurrentBmcSession),
                },
            },
        };

    std::string systemUuid{""};
    uint64_t jsonRevision = 1;

    // set the permission of the file to 640
    std::filesystem::perms configPermission =
        std::filesystem::perms::owner_read |
        std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read;
};

inline ConfigFile& getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace config
} // namespace service
} // namespace app
#endif // __CONFIGURATION_H__
