#include <iostream>
#include "Logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Logger::Logger(const std::string& logFile) {
    try {
        // Create a multi-sink logger with both console and file outputs
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile, 1024 * 1024 * 5, 3); // 5MB size, 3 rotations
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
        m_logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
        m_logger->set_level(spdlog::level::debug);
        m_logger->flush_on(spdlog::level::info);

        // Set pattern: [time] [level] message
        m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog::register_logger(m_logger);
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

Logger::~Logger() {
    if (m_logger) {
        m_logger->flush();
        spdlog::drop_all(); // This releases all loggers
    }
}

void Logger::Debug(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logger) m_logger->debug(message);
}

void Logger::Info(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logger) m_logger->info(message);
}

void Logger::Warning(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logger) m_logger->warn(message);
}

void Logger::Error(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logger) m_logger->error(message);
}

void Logger::Critical(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logger) m_logger->critical(message);
}
