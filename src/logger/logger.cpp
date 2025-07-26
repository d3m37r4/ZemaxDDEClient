// #include <mutex>  // для многопоточности
#include "logger.h"

// Initialization of the Logger class.
Logger::Logger() {}

void Logger::addLog(const std::string& message) {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", ltm);
    logs.push_back(std::string(timestamp) + " " + message);
}

const std::vector<std::string>& Logger::getLogs() const {
    return logs;
}
