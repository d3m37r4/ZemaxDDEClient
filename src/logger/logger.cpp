#include <ctime>
#include <cstring>
#include <filesystem>
#include <format>

#ifdef DEBUG_LOG
#include <iostream>
#endif

#include <windows.h>
#include <shlobj.h>

#include "logger.h"

#include "app/config_path.h"

namespace {
    std::string getLocalAppDataPath() {
        static std::string path;
        if (path.empty()) {
            PWSTR localAppDataPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataPath))) {
                std::wstring configFolder = std::wstring(localAppDataPath) + L"\\ZemaxDDEClient";
                CoTaskMemFree(localAppDataPath);

                int utf8Length = WideCharToMultiByte(CP_UTF8, 0, configFolder.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (utf8Length > 0) {
                    path.resize(utf8Length - 1);
                    WideCharToMultiByte(CP_UTF8, 0, configFolder.c_str(), -1, &path[0], utf8Length, nullptr, nullptr);
                }
            }
        }
        return path;
    }
}

std::string Logger::getLogDir() const {
    // Single source of truth: Preferences -> Files shows this path and the
    // "clean log folder" action deletes from it.
    const char* configured = app::getLogFolderPath();
    if (configured && *configured) {
        return configured;
    }
    std::string basePath = getLocalAppDataPath();
    if (basePath.empty()) return "logs";
    return basePath + "\\logs";
}

std::string Logger::getLogPath(const std::string& date, int rotation) const {
    std::string dir = getLogDir();
    if (rotation == 0) {
        return dir + "\\L" + date + ".log";
    }
    char suffix[8];
    std::snprintf(suffix, sizeof(suffix), "-%02d", rotation);
    return dir + "\\L" + date + suffix + ".log";
}

void Logger::openLogFile(const tm& ltm) {
    char dateBuf[16];
    std::strftime(dateBuf, sizeof(dateBuf), "%d%m%Y", &ltm);
    std::string today(dateBuf);

    // The file is already open for today: skip the directory work below, which
    // otherwise ran a filesystem syscall on every single log line.
    if (today == m_currentLogDate && m_logFile.is_open()) {
        return;
    }

    std::string logDir = getLogDir();
    std::filesystem::create_directories(logDir);

    m_currentLogDate = today;

    // First slot that still has room; existing archives are never truncated
    // (the old code reopened -01.log with trunc and erased the previous
    // session's log on a restart within the same day).
    std::string path;
    m_rotationIndex = 0;
    for (int rotation = 0; rotation <= 999; ++rotation) {
        const std::string candidate = getLogPath(today, rotation);
        std::error_code ec;
        const auto size = std::filesystem::file_size(candidate, ec);
        if (ec || size < m_maxFileSize) {
            m_rotationIndex = rotation;
            path = candidate;
            break;
        }
    }
    if (path.empty()) {
        path = getLogPath(today, 0);
    }
    m_logFile.open(path, std::ios::app);
    m_unflushedLines = 0;
    if (m_logFile.is_open()) {
        m_logFile.seekp(0, std::ios::end);
        m_currentFileSize = static_cast<size_t>(m_logFile.tellp());
    }
}

void Logger::rotateLogFile() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_unflushedLines = 0;
    // Skip to the first slot with room instead of truncating an existing
    // archive: a restart within the same day used to erase -01.log.
    std::string path;
    for (int rotation = m_rotationIndex + 1; rotation <= 999; ++rotation) {
        const std::string candidate = getLogPath(m_currentLogDate, rotation);
        std::error_code ec;
        const auto size = std::filesystem::file_size(candidate, ec);
        if (ec || size < m_maxFileSize) {
            m_rotationIndex = rotation;
            path = candidate;
            break;
        }
    }
    if (path.empty()) {
        ++m_rotationIndex;
        path = getLogPath(m_currentLogDate, m_rotationIndex);
    }
    m_logFile.open(path, std::ios::app);
    if (m_logFile.is_open()) {
        m_logFile.seekp(0, std::ios::end);
        m_currentFileSize = static_cast<size_t>(m_logFile.tellp());
    } else {
        m_currentFileSize = 0;
    }
}

void Logger::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Turning file logging off has to persist the tail: nothing flushes the
    // stream afterwards.
    if (!enabled && m_logFile.is_open() && m_unflushedLines > 0) {
        m_logFile.flush();
        m_unflushedLines = 0;
    }
    m_enabled = enabled;
}

std::string Logger::getCurrentLogPath() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return getLogPath(m_currentLogDate, m_rotationIndex);
}

void Logger::clearLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
    ++m_epoch;
    m_droppedPrefix = 0;
    m_revision.fetch_add(1, std::memory_order_relaxed);
}

std::vector<std::string> Logger::getLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return {m_logs.begin(), m_logs.end()};
}

size_t Logger::syncLogs(std::vector<std::string>& entries, LogSyncCursor& cursor) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (cursor.epoch != m_epoch || m_droppedPrefix < cursor.droppedPrefix) {
        entries.assign(m_logs.begin(), m_logs.end());
        cursor.epoch = m_epoch;
        cursor.droppedPrefix = m_droppedPrefix;
        return static_cast<size_t>(-1);
    }

    const size_t trimmed = m_droppedPrefix - cursor.droppedPrefix;
    if (trimmed > 0) {
        if (trimmed >= entries.size()) {
            entries.clear();
        } else {
            entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(trimmed));
        }
    }

    if (entries.size() > m_logs.size()) {
        entries.assign(m_logs.begin(), m_logs.end());
        cursor.droppedPrefix = m_droppedPrefix;
        return static_cast<size_t>(-1);
    }

    const size_t start = entries.size();
    for (size_t i = start; i < m_logs.size(); ++i) {
        entries.push_back(m_logs[i]);
    }
    cursor.droppedPrefix = m_droppedPrefix;
    return trimmed;
}

bool Logger::tryCopyLogTail(char* buffer, size_t capacity, size_t& written) const {
    written = 0;
    if (buffer == nullptr || capacity == 0) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return false;
    }

    // Walk from the end so that the most recent lines survive the capacity
    // limit, then copy them in chronological order.
    size_t first = m_logs.size();
    size_t required = 0;
    while (first > 0) {
        const size_t entrySize = m_logs[first - 1].size() + 1;
        if (required + entrySize > capacity) {
            break;
        }
        required += entrySize;
        --first;
    }

    size_t used = 0;
    for (size_t i = first; i < m_logs.size(); ++i) {
        const std::string& entry = m_logs[i];
        std::memcpy(buffer + used, entry.data(), entry.size());
        used += entry.size();
        buffer[used++] = '\n';
    }
    written = used;
    return true;
}

void Logger::addLog(std::string_view message) {
    std::string logEntry;
    tm ltm{};
    bool hasLocalTime = false;

    try {
        time_t now = std::time(nullptr);
        if (localtime_s(&ltm, &now) == 0) {
            hasLocalTime = true;
            char timestamp[32];
            if (std::strftime(timestamp, sizeof(timestamp), TIME_FORMAT.data(), &ltm) > 0) {
                logEntry = std::format("{} {}", timestamp, message);
            } else {
                logEntry = std::format("[strftime failed] {}", message);
            }
        } else {
            logEntry = std::format("[localtime failed] {}", message);
        }
    } catch (...) {
        logEntry = std::format("[log timestamp exception] {}", message);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back(std::move(logEntry));
    // Bound the in-memory buffer: a precise map run appends tens of thousands
    // of lines. Trim in batches so the erase cost stays amortized.
    if (m_logs.size() > MAX_MEMORY_LOGS) {
        m_logs.erase(m_logs.begin(), m_logs.begin() + static_cast<std::ptrdiff_t>(TRIM_BATCH));
        m_droppedPrefix += TRIM_BATCH;
    }
    m_revision.fetch_add(1, std::memory_order_relaxed);

    #ifdef DEBUG_LOG
    std::cout << m_logs.back() << '\n';
    #endif

    if (!m_enabled || !hasLocalTime) return;

    openLogFile(ltm);
    if (m_logFile.is_open()) {
        m_logFile << m_logs.back() << '\n';
        m_currentFileSize += m_logs.back().size() + 1;
        ++m_unflushedLines;
        if (m_unflushedLines >= FLUSH_BATCH) {
            m_logFile.flush();
            m_unflushedLines = 0;
        }
        if (m_currentFileSize >= m_maxFileSize) {
            rotateLogFile();
        }
    }
}
