#pragma once
#include <string>
#include <fstream>

namespace gudb::core {
    enum class LogLevel { DEBUG, INFO, WARN, ERROR };

    class Logger {
    public:
        static Logger &instance();

        void log(LogLevel level, const std::string &msg);

    private:
        Logger();

        std::ofstream logFile_;
    };

#define LOG_DEBUG(msg) gudb::core::Logger::instance().log(gudb::core::LogLevel::DEBUG, msg)
#define LOG_INFO(msg)  gudb::core::Logger::instance().log(gudb::core::LogLevel::INFO, msg)
#define LOG_WARN(msg)  gudb::core::Logger::instance().log(gudb::core::LogLevel::WARN, msg)
#define LOG_ERROR(msg) gudb::core::Logger::instance().log(gudb::core::LogLevel::ERROR, msg)
} // namespace gudb::core
