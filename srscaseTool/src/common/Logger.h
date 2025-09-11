#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

class Logger {
public:
    Logger(const std::string& logFile);
    ~Logger();

    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::mutex m_mutex;
};
