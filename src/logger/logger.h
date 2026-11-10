#pragma once

#include <atomic>
#include <deque>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>

class Logger {
    public:
        void addLog(std::string_view message);

        /// Thread-safe: removes all entries under the mutex.
        void clearLogs();

        /// Thread-safe: returns a snapshot copy of all entries taken under the mutex,
        /// so a reader can never observe the container mid-mutation.
        [[nodiscard]] std::vector<std::string> getLogs() const;

        /// Monotonic counter bumped on every append/clear: the Logs panel skips
        /// the whole-buffer snapshot copy on frames where nothing changed.
        [[nodiscard]] uint64_t revision() const noexcept {
            return m_revision.load(std::memory_order_relaxed);
        }

        void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
        void setMaxFileSize(size_t bytes) noexcept { m_maxFileSize = bytes; }
        [[nodiscard]] std::string getCurrentLogPath() const;

    private:
        /// In-memory ceiling: a precise map run appends tens of thousands of
        /// lines; unbounded growth slows every frame and eventually the process.
        /// Oldest entries are dropped in batches so trimming stays amortized.
        static constexpr size_t MAX_MEMORY_LOGS = 20000;
        static constexpr size_t TRIM_BATCH = 2000;

        std::deque<std::string> m_logs;
        std::atomic<uint64_t> m_revision{0};
        mutable std::mutex m_mutex;
        static constexpr std::string_view TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";

        std::ofstream m_logFile;
        size_t m_currentFileSize = 0;
        size_t m_maxFileSize = 5 * 1024 * 1024;
        bool m_enabled = false;
        std::string m_currentLogDate;
        int m_rotationIndex = 0;

        void openLogFile();
        void rotateLogFile();
        std::string getLogDir() const;
        std::string getLogPath(const std::string& date, int rotation) const;
};
