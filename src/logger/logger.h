#pragma once

#include <atomic>
#include <ctime>
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

        struct LogSyncCursor {
            uint64_t epoch = 0;
            size_t droppedPrefix = 0;
        };

        /// Incrementally updates @p entries with lines appended since the last
        /// call. Returns SIZE_MAX when @p entries was rebuilt from scratch
        /// (first call or after clearLogs()), or the number of oldest entries
        /// trimmed from the front of @p entries before appending the new tail.
        size_t syncLogs(std::vector<std::string>& entries, LogSyncCursor& cursor) const;

        /// Copies the tail of the buffered entries into @p buffer without
        /// allocating and without blocking. The crash handler uses it: a
        /// crashed process may hold the mutex, and a blocking lock there would
        /// deadlock the report. Returns false when the mutex is held.
        [[nodiscard]] bool tryCopyLogTail(char* buffer, size_t capacity, size_t& written) const;

        void setEnabled(bool enabled);
        void setMaxFileSize(size_t bytes) noexcept { m_maxFileSize = bytes; }
        [[nodiscard]] std::string getCurrentLogPath() const;

    private:
        /// In-memory ceiling: a precise map run appends tens of thousands of
        /// lines; unbounded growth slows every frame and eventually the process.
        /// Oldest entries are dropped in batches so trimming stays amortized.
        static constexpr size_t MAX_MEMORY_LOGS = 20000;
        static constexpr size_t TRIM_BATCH = 2000;
        /// File flush cadence: flushing on every line costs thousands of
        /// synchronous WriteFile calls on the DDE/UI thread during a map run.
        /// A crash report reads the in-memory tail through tryCopyLogTail(),
        /// so an unflushed file buffer cannot lose the diagnostics.
        static constexpr size_t FLUSH_BATCH = 64;

        std::deque<std::string> m_logs;
        uint64_t m_epoch = 1;
        size_t m_droppedPrefix = 0;
        std::atomic<uint64_t> m_revision{0};
        mutable std::mutex m_mutex;
        static constexpr std::string_view TIME_FORMAT = "[%d.%m.%Y - %H:%M:%S]";

        std::ofstream m_logFile;
        size_t m_currentFileSize = 0;
        size_t m_maxFileSize = 5 * 1024 * 1024;
        size_t m_unflushedLines = 0;
        bool m_enabled = false;
        std::string m_currentLogDate;
        int m_rotationIndex = 0;

        void openLogFile(const tm& ltm);
        void rotateLogFile();
        std::string getLogDir() const;
        std::string getLogPath(const std::string& date, int rotation) const;
};
