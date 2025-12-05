#include "Logger.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>

namespace gudb::core {
    Logger::Logger() {
        logFile_.open("gudb.log", std::ios::app);
    }

    Logger &Logger::instance() {
        static Logger instance;
        return instance;
    }

    void Logger::log(LogLevel level, const std::string &msg) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        const char *levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR"};

        std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
                << "[" << levelStr[static_cast<int>(level)] << "] "
                << msg << std::endl;

        logFile_ << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
                << "[" << levelStr[static_cast<int>(level)] << "] "
                << msg << std::endl;
        logFile_.flush();
    }
} // namespace gudb::core