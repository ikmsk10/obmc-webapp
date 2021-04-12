// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace app
{

enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical,
    Alert,
    Emerg
};

class Logger
{
  private:
    //
    static std::string timestamp()
    {
        std::string date;
        date.resize(32, '\0');
        time_t t = time(nullptr);

        tm myTm{};

        gmtime_r(&t, &myTm);

        size_t sz =
            strftime(date.data(), date.size(), "%Y-%m-%d %H:%M:%S", &myTm);
        date.resize(sz);
        return date;
    };

  public:
    Logger([[maybe_unused]] const std::string& prefix,
           [[maybe_unused]] const std::string& filename,
           [[maybe_unused]] const size_t line, LogLevel levelIn) :
        level(levelIn)
    {
        stringstream << "(" << timestamp() << ") [" << prefix << " "
                     << std::filesystem::path(filename).filename() << ":"
                     << line << "] ";
    }
    ~Logger()
    {
        if (level >= getCurrentLogLevel())
        {
            stringstream << std::endl;
            std::cerr << stringstream.str();
        }
    }

    template <typename T>
    Logger& operator<<([[maybe_unused]] T const& value)
    {
        if (level >= getCurrentLogLevel())
        {
            stringstream << value;
        }
        return *this;
    }

    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel getCurrentLogLevel()
    {
        return getLogLevelRef();
    }

  private:
    //
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = static_cast<LogLevel>(0);
        return currentLevel;
    }

    //
    std::ostringstream stringstream;
    app::LogLevel level;
};

#define LOG_EMERG                                                              \
    app::Logger("EMERGENCY", __FILE__, __LINE__, app::LogLevel::Emerg)
#define LOG_ALERT app::Logger("ALERT", __FILE__, __LINE__, app::LogLevel::Alert)
#define LOG_CRITICAL                                                           \
    app::Logger("CRITICAL", __FILE__, __LINE__, app::LogLevel::Critical)
#define LOG_ERROR app::Logger("ERROR", __FILE__, __LINE__, app::LogLevel::Error)
#define LOG_WARNING                                                            \
    app::Logger("WARNING", __FILE__, __LINE__, app::LogLevel::Warning)
#define LOG_INFO app::Logger("INFO", __FILE__, __LINE__, app::LogLevel::Info)
#define LOG_DEBUG app::Logger("DEBUG", __FILE__, __LINE__, app::LogLevel::Debug)

} // namespace app
#endif // __LOGGER_H__
