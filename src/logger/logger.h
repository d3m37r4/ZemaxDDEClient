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
};
#endif
