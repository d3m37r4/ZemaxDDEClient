// #include <mutex>  // Для многопоточности
#include <ctime>
#include "logger.h"

const char* Logger::LOG_TIME_FORMAT = "[%Y-%m-%d %H:%M:%S]";        // Define format of log time output

// Initialization of the Logger class.
Logger::Logger() {}

void Logger::addLog(const std::string& message) {
    time_t now = time(0);
    tm ltm;
    setlocale(LC_TIME, "");     // Устанавливаем локаль, на всякий случай
    if (localtime_s(&ltm, &now) == 0) {
        char timestamp[25];
        if (strftime(timestamp, sizeof(timestamp), "[%H:%M:%S - %d.%m.%Y]", &ltm) > 0) {
            logs.push_back(std::string(timestamp) + " " + message);
        } else {
            logs.push_back("[strftime failed] " + message);
        }
    } else {
        logs.push_back("[localtime failed] " + message);
    }
}

const std::vector<std::string>& Logger::getLogs() const {
    return logs;
}
