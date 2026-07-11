#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>

class Logger {
    public:
        void addLog(std::string_view message);
        void clearLogs() noexcept { m_logs.clear(); }
        [[nodiscard]] const std::vector<std::string>& getLogs() const noexcept { return m_logs; }

        void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
        void setMaxFileSize(size_t bytes) noexcept { m_maxFileSize = bytes; }
        [[nodiscard]] std::string getCurrentLogPath() const;

    private:
        std::vector<std::string> m_logs;
        mutable std::mutex m_mutex;
        static constexpr std::string_view TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";

        std::ofstream m_logFile;
        size_t m_currentFileSize = 0;
        size_t m_maxFileSize = 5 * 1024 * 1024;
        bool m_enabled = true;
        std::string m_currentLogDate;
        int m_rotationIndex = 0;

        void openLogFile();
        void rotateLogFile();
        std::string getLogDir() const;
        std::string getLogPath(const std::string& date, int rotation) const;
};
