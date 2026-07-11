#pragma once

#include <string>

namespace app {
    const char* getImguiIniPath();
    const char* getWindowStatePath();
    const char* getSettingsJsonPath();
    const char* getLogFolderPath();
    std::string getLogFolderSize();
    size_t cleanLogFolder(const std::string& skipFilePath);
}