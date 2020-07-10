#pragma once

#include <any>
#include <string>
#include <typeinfo>
#include <iostream>

enum class LogLevel : uint8_t
{
    Critical = 1,
    Error    = 2,
    Warning  = 4,
    Info     = 8,
    Debug    = 16,
};

constexpr LogLevel operator|(LogLevel X, LogLevel Y) {
    return static_cast<LogLevel>(
        static_cast<uint8_t>(X) | static_cast<uint8_t>(Y));
}

class Logger
{
public:
    static void Log(std::any object, LogLevel level, std::string message)
    {
        std::string objectName = object.type().name();
        if ((currentLogLevel & uint32_t(level)) > 0)
        {
            std::cout << objectName << ": " << message << std::endl;
        }
    }

    static void LogDebug(std::any object, std::string message)
    {
        Logger::Log(object, LogLevel::Debug, message);
    }

    static void LogInfo(std::any object, std::string message)
    {
        Logger::Log(object, LogLevel::Info, message);
    }

    static void LogWarning(std::any object, std::string message)
    {
        Logger::Log(object, LogLevel::Warning, message);
    }

    static void LogError(std::any object, std::string message)
    {
        Logger::Log(object, LogLevel::Error, message);
    }

    static void LogCritical(std::any object, std::string message)
    {
        Logger::Log(object, LogLevel::Critical, message);
    }

    static void SetLogLevel(LogLevel level)
    {
        Logger::currentLogLevel = static_cast<uint8_t>(level);
    }

private:
    static inline uint8_t currentLogLevel;
};