#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <mutex>

class Logger {
    public:
        void addLog(std::string_view message);
        void clearLogs() noexcept { m_logs.clear(); }
        [[nodiscard]] const std::vector<std::string>& getLogs() const noexcept { return m_logs; }

    private:
        std::vector<std::string> m_logs;
        std::mutex m_mutex;
        static constexpr std::string_view TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";
};
