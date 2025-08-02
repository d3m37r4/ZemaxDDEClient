#include <ctime>
#ifdef DEBUG_LOG
    #include <iostream>
#endif
#include "logger.h"

const char* Logger::LOG_TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";      // Define format of log time output

Logger::Logger() {}                                                 // Initialization of the Logger class.

void Logger::addLog(const std::string& message) {
    time_t now = time(0);
    tm ltm;
    setlocale(LC_TIME, "");     // Set locale (just in case)
    std::string logEntry;
    if (localtime_s(&ltm, &now) == 0) {
        char timestamp[25];
        if (strftime(timestamp, sizeof(timestamp), LOG_TIME_FORMAT, &ltm) > 0) {
            logEntry = std::string(timestamp) + " " + message;
        } else {
            logEntry = "[strftime failed] " + message;
        }
    } else {
        logEntry = "[localtime failed] " + message;
    }
    logs.push_back(logEntry);
#ifdef DEBUG_LOG
    std::cout << logEntry << std::endl;                             // Immediate output to console only in debug mode
#endif
}

const std::vector<std::string>& Logger::getLogs() const {
    return logs;
}
