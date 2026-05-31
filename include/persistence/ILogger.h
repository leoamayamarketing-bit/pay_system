#ifndef FREEBUFF_ILOGGER_H
#define FREEBUFF_ILOGGER_H

#include <string>
#include <memory>

namespace freebuff::persist {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, const std::string& message) = 0;

    virtual void debug(const std::string& message) { log(LogLevel::Debug, message); }
    virtual void info(const std::string& message) { log(LogLevel::Info, message); }
    virtual void warning(const std::string& message) { log(LogLevel::Warning, message); }
    virtual void error(const std::string& message) { log(LogLevel::Error, message); }
    virtual void critical(const std::string& message) { log(LogLevel::Critical, message); }

    [[nodiscard]] virtual std::string logLevelToString(LogLevel level) const noexcept;
};

// Factory functions
std::unique_ptr<ILogger> createConsoleLogger(LogLevel minLevel = LogLevel::Info);
std::unique_ptr<ILogger> createFileLogger(const std::string& filePath, LogLevel minLevel = LogLevel::Info);

} // namespace freebuff::persist

#endif // FREEBUFF_ILOGGER_H
