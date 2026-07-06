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

    private:
        std::vector<std::string> m_logs;
        std::mutex m_mutex;
        static constexpr std::string_view TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";

        std::ofstream m_logFile;
        size_t m_currentFileSize = 0;
        std::string m_currentLogDate;
        int m_rotationIndex = 0;

        static constexpr size_t MAX_FILE_SIZE = 5 * 1024 * 1024;

        void openLogFile();
        void rotateLogFile();
        std::string getLogDir() const;
        std::string getLogPath(const std::string& date, int rotation) const;
};
