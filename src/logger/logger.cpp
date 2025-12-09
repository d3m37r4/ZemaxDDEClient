#ifdef DEBUG_LOG
    #include <iostream>
#endif

#include "logger.h"

void Logger::addLog(const std::string& message) {
    std::string logEntry;
    
    try {
        time_t now = std::time(nullptr);
        tm ltm{};
        if (localtime_s(&ltm, &now) == 0) {
            char timestamp[32];
            if (std::strftime(timestamp, sizeof(timestamp), timeFormat.data(), &ltm) > 0) {
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

    logs.push_back(std::move(logEntry));

#ifdef DEBUG_LOG
    std::cout << logs.back() << '\n';
#endif
}
