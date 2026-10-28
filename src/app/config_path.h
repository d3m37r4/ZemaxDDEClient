#pragma once

#include <string>

namespace app {
    const char* getImguiIniPath();
    const char* getWindowStatePath();
    const char* getChromeStatePath();
    const char* getSettingsJsonPath();
    const char* getLogFolderPath();
    std::string getLogFolderSize();
    size_t cleanLogFolder(const std::string& skipFilePath);

    /// Redirect every persisted file (settings.json, imgui.ini, windows.json
    /// and the logs folder) into @p dir. Used by the GUI test harness so a
    /// test run cannot touch the user's real profile. Pass an empty string to
    /// restore the default locations. Has no effect once a path was resolved.
    ///
    /// Only available in builds with the test engine enabled; in regular
    /// builds this is not compiled at all.
#ifdef ZEMAX_ENABLE_TEST_ENGINE
    void setConfigDirOverride(const std::string& dir);
#endif
}