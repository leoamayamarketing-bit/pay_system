#include "persistence/ILogger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <mutex>

namespace freebuff::persist {

std::string ILogger::logLevelToString(LogLevel level) const noexcept {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
    }
    return "UNKNOWN";
}

class ConsoleLogger : public ILogger {
public:
    explicit ConsoleLogger(LogLevel minLevel = LogLevel::Info)
        : minLevel_(minLevel) {}

    void log(LogLevel level, const std::string& message) override {
        if (level < minLevel_) return;
        
        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm nowTm;
        localtime_s(&nowTm, &nowTime);
        
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[" << logLevelToString(level) << "] "
                  << std::put_time(&nowTm, "%H:%M:%S") << " "
                  << message << std::endl;
    }

private:
    LogLevel minLevel_;
    std::mutex mutex_;
};

class FileLogger : public ILogger {
public:
    FileLogger(const std::string& filePath, LogLevel minLevel = LogLevel::Info)
        : minLevel_(minLevel)
        , filePath_(filePath)
    {
        file_.open(filePath_, std::ios::app);
    }

    ~FileLogger() override {
        if (file_.is_open()) file_.close();
    }

    void log(LogLevel level, const std::string& message) override {
        if (level < minLevel_) return;
        
        auto now = std::chrono::system_clock::now();
        auto nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm nowTm;
        localtime_s(&nowTm, &nowTime);
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_ << "[" << logLevelToString(level) << "] "
                  << std::put_time(&nowTm, "%Y-%m-%d %H:%M:%S") << " "
                  << message << std::endl;
            file_.flush();
        }
    }

private:
    LogLevel minLevel_;
    std::string filePath_;
    std::ofstream file_;
    std::mutex mutex_;
};

std::unique_ptr<ILogger> createConsoleLogger(LogLevel minLevel) {
    return std::make_unique<ConsoleLogger>(minLevel);
}

std::unique_ptr<ILogger> createFileLogger(const std::string& filePath, LogLevel minLevel) {
    return std::make_unique<FileLogger>(filePath, minLevel);
}

} // namespace freebuff::persist
