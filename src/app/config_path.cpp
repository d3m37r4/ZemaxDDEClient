#include "app/config_path.h"

#include <filesystem>
#include <windows.h>
#include <shlobj.h>

#include "app/app.h"

namespace {
    inline constexpr const char* IMGUI_INI_FILENAME = "imgui.ini";
    inline constexpr const char* WINDOW_STATE_FILENAME = "windows.json";

    std::string imguiIniPath;
    std::string windowStatePath;

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
}

namespace app {
    const char* getImguiIniPath() {
        if (imguiIniPath.empty()) {
#ifdef APP_PRODUCTION_BUILD
            std::string basePath = getLocalAppDataPath();
            if (!basePath.empty()) {
                imguiIniPath = basePath + "\\" + std::string(IMGUI_INI_FILENAME);
            } else {
                imguiIniPath = IMGUI_INI_FILENAME;
            }
#else
            imguiIniPath = IMGUI_INI_FILENAME;
#endif
        }
        return imguiIniPath.c_str();
    }

    const char* getWindowStatePath() {
        if (windowStatePath.empty()) {
#ifdef APP_PRODUCTION_BUILD
            std::string basePath = getLocalAppDataPath();
            if (!basePath.empty()) {
                windowStatePath = basePath + "\\" + std::string(WINDOW_STATE_FILENAME);
            } else {
                windowStatePath = WINDOW_STATE_FILENAME;
            }
#else
            windowStatePath = WINDOW_STATE_FILENAME;
#endif
        }
        return windowStatePath.c_str();
    }
}