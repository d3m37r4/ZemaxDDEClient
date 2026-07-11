#include "app/config_path.h"

#include <filesystem>
#include <windows.h>
#include <shlobj.h>

#include "app/app.h"
#include "app/utils.h"

namespace {
    inline constexpr const char* IMGUI_INI_FILENAME = "imgui.ini";
    inline constexpr const char* WINDOW_STATE_FILENAME = "windows.json";
    inline constexpr const char* SETTINGS_JSON_FILENAME = "settings.json";

    std::string imguiIniPath;
    std::string windowStatePath;
    std::string settingsJsonPath;
    std::string logFolderPath;

    const char* getLocalAppDataPath() {
        static std::string path;
        if (path.empty()) {
            PWSTR localAppDataPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataPath))) {
                std::wstring appNameWide = L"ZemaxDDEClient";
                std::wstring configFolder = std::wstring(localAppDataPath) + L"\\" + appNameWide;
                CoTaskMemFree(localAppDataPath);

                std::filesystem::create_directories(configFolder);

                int utf8Length = WideCharToMultiByte(CP_UTF8, 0, configFolder.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (utf8Length > 0) {
                    path.resize(utf8Length - 1);
                    WideCharToMultiByte(CP_UTF8, 0, configFolder.c_str(), -1, &path[0], utf8Length, nullptr, nullptr);
                }
            }
        }
        return path.c_str();
    }

    std::string resolvePath(const char* filename) {
        #ifdef APP_PRODUCTION_BUILD
        std::string basePath = getLocalAppDataPath();
        if (!basePath.empty()) {
            return basePath + "\\" + std::string(filename);
        }
        return std::string(filename);
        #else
        return std::string(filename);
        #endif
    }
}

namespace app {
    const char* getImguiIniPath() {
        if (imguiIniPath.empty()) {
            imguiIniPath = resolvePath(IMGUI_INI_FILENAME);
        }
        return imguiIniPath.c_str();
    }

    const char* getWindowStatePath() {
        if (windowStatePath.empty()) {
            windowStatePath = resolvePath(WINDOW_STATE_FILENAME);
        }
        return windowStatePath.c_str();
    }

    const char* getSettingsJsonPath() {
        if (settingsJsonPath.empty()) {
            settingsJsonPath = resolvePath(SETTINGS_JSON_FILENAME);
        }
        return settingsJsonPath.c_str();
    }

    const char* getLogFolderPath() {
        if (logFolderPath.empty()) {
            std::string basePath = getLocalAppDataPath();
            if (!basePath.empty()) {
                logFolderPath = basePath + "\\logs";
            }
        }
        return logFolderPath.c_str();
    }

    std::string getLogFolderSize() {
        const char* path = getLogFolderPath();
        if (!path || path[0] == '\0') return "0 B";

        uintmax_t totalSize = 0;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (entry.is_regular_file(ec))
                totalSize += entry.file_size(ec);
        }
        return formatFileSize(totalSize);
    }

    size_t cleanLogFolder(const std::string& skipFilePath) {
        const char* path = getLogFolderPath();
        if (!path || path[0] == '\0') return 0;

        size_t deleted = 0;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            if (entry.path() == skipFilePath) continue;
            std::filesystem::remove(entry.path(), ec);
            if (!ec) deleted++;
        }
        return deleted;
    }
}