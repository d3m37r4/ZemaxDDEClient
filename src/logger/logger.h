#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <ctime>

class Logger {
    public:
        Logger();
        void addLog(const std::string& message);
        const std::vector<std::string>& getLogs() const;

    private:
        std::vector<std::string> logs;
        static const char* LOG_TIME_FORMAT;     // Time format const
    };
#endif
