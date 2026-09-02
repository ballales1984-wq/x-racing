#pragma once

#include <string>

namespace xe {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    static void Init();
    static void Shutdown();
    static void Log(LogLevel level, const std::string& message);

    static void Debug(const std::string& msg);
    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);

private:
    static bool initialized_;
};

}  // namespace xe

#define XE_LOG_DEBUG(msg) xe::Logger::Debug(msg)
#define XE_LOG_INFO(msg)  xe::Logger::Info(msg)
#define XE_LOG_WARN(msg)  xe::Logger::Warn(msg)
#define XE_LOG_ERROR(msg) xe::Logger::Error(msg)
