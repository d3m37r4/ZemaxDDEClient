#include "app/config_path.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>

#include "app/app.h"

namespace {
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
                imguiIniPath = basePath + "\\imgui.ini";
            } else {
                imguiIniPath = "imgui.ini";
            }
#else
            imguiIniPath = "imgui.ini";
#endif
        }
        return imguiIniPath.c_str();
    }

    const char* getWindowStatePath() {
        if (windowStatePath.empty()) {
#ifdef APP_PRODUCTION_BUILD
            std::string basePath = getLocalAppDataPath();
            if (!basePath.empty()) {
                windowStatePath = basePath + "\\windows.json";
            } else {
                windowStatePath = "windows.json";
            }
#else
            windowStatePath = "windows.json";
#endif
        }
        return windowStatePath.c_str();
    }
}