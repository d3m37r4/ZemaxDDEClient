#include <ctime>
#include <filesystem>
#include <format>

#ifdef DEBUG_LOG
#include <iostream>
#endif

#include <windows.h>
#include <shlobj.h>

#include "logger.h"

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

void Logger::openLogFile() {
    time_t now = std::time(nullptr);
    tm ltm{};
    localtime_s(&ltm, &now);

    char dateBuf[16];
    std::strftime(dateBuf, sizeof(dateBuf), "%d%m%Y", &ltm);
    std::string today(dateBuf);

    std::string logDir = getLogDir();
    std::filesystem::create_directories(logDir);

    if (today == m_currentLogDate && m_logFile.is_open()) {
        return;
    }

    m_currentLogDate = today;
    m_rotationIndex = 0;

    std::string path = getLogPath(today, 0);
    m_logFile.open(path, std::ios::app);
    if (m_logFile.is_open()) {
        m_logFile.seekp(0, std::ios::end);
        m_currentFileSize = static_cast<size_t>(m_logFile.tellp());
    }
}

void Logger::rotateLogFile() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_rotationIndex++;
    std::string path = getLogPath(m_currentLogDate, m_rotationIndex);
    m_logFile.open(path, std::ios::trunc);
    m_currentFileSize = 0;
}

void Logger::addLog(std::string_view message) {
    std::string logEntry;

    try {
        time_t now = std::time(nullptr);
        tm ltm{};
        if (localtime_s(&ltm, &now) == 0) {
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

    #ifdef DEBUG_LOG
    std::cout << m_logs.back() << '\n';
    #endif

    openLogFile();
    if (m_logFile.is_open()) {
        m_logFile << m_logs.back() << '\n';
        m_logFile.flush();
        m_currentFileSize += m_logs.back().size() + 1;
        if (m_currentFileSize >= MAX_FILE_SIZE) {
            rotateLogFile();
        }
    }
}
