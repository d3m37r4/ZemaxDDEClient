#pragma once

#include <string>
#include <vector>
#include <string_view>

class Logger {
    public:
        void addLog(const std::string& message);
        void clearLogs() noexcept { logs.clear(); }
        const std::vector<std::string>& getLogs() const noexcept { return logs; }

    private:
        std::vector<std::string> logs;
        static constexpr std::string_view timeFormat = "[%d.%m.%Y - %H:%M:%S]";
};

extern Logger logger;
