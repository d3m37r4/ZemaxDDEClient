#include <ctime>
#include <format>

#ifdef DEBUG_LOG
    #include <iostream>
#endif

#include "logger.h"

void Logger::addLog(std::string_view message) {
    std::string logEntry;

    try {
        time_t now = std::time(nullptr);
        tm ltm{};
        if (localtime_s(&ltm, &now) == 0) {
            char timestamp[32];
            if (std::strftime(timestamp, sizeof(timestamp), TIME_FORMAT.data(), &ltm) > 0) {
                logEntry = std::format("{} {}", timestamp, message);
            } else {
                logEntry = std::format("[strftime failed] {}", message);
            }
        } else {
            logEntry = std::format("[localtime failed] {}", message);
        }
    } catch (...) {
        logEntry = std::format("[log timestamp exception] {}", message);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back(std::move(logEntry));

#ifdef DEBUG_LOG
    std::cout << m_logs.back() << '\n';
#endif
}
