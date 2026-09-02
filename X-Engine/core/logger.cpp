#include "core/logger.h"
#include <iostream>

namespace xe {

bool Logger::initialized_ = false;

void Logger::Init() {
    if (initialized_) return;
    initialized_ = true;
    std::cout << "[xe] Logger initialized\n";
    std::cout.flush();
}

void Logger::Shutdown() {
    if (!initialized_) return;
    std::cout << "[xe] Logger shutdown\n";
    std::cout.flush();
    initialized_ = false;
}

static const char* LevelPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "[DEBUG]";
        case LogLevel::Info:  return "[INFO] ";
        case LogLevel::Warn:  return "[WARN] ";
        case LogLevel::Error: return "[ERROR]";
        default:              return "[????] ";
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (!initialized_) return;
    std::cout << LevelPrefix(level) << " " << message << "\n";
    std::cout.flush();
}

void Logger::Debug(const std::string& msg) { Log(LogLevel::Debug, msg); }
void Logger::Info(const std::string& msg)  { Log(LogLevel::Info, msg); }
void Logger::Warn(const std::string& msg)  { Log(LogLevel::Warn, msg); }
void Logger::Error(const std::string& msg) { Log(LogLevel::Error, msg); }

}  // namespace xe
