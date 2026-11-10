#include "test_engine/test_profile.h"

#include "app/config_path.h"

#include <windows.h>

#include <filesystem>
#include <string>

namespace app::testing {

namespace {

std::string g_profilePath;

// Builds a unique directory under %TEMP% so parallel test runs cannot clash.
std::string makeProfileDir() {
    namespace fs = std::filesystem;

    const char* tempEnv = std::getenv("TEMP");
    fs::path base = (tempEnv && tempEnv[0]) ? fs::path(tempEnv) : fs::temp_directory_path();
    if (base.empty()) {
        base = fs::path(".");
    }

    const DWORD pid = GetCurrentProcessId();

    // A few attempts in case a previous run left the directory behind.
    for (int attempt = 0; attempt < 8; ++attempt) {
        fs::path candidate = base / ("ZemaxDDEClient_tests_" + std::to_string(pid) + "_" + std::to_string(attempt));
        std::error_code ec;
        if (fs::create_directories(candidate, ec) && !ec) {
            return candidate.string();
        }
        // Directory already exists: try the next suffix.
    }
    return {};
}

} // namespace

bool useIsolatedTestProfile() {
    std::string dir = makeProfileDir();
    if (dir.empty()) {
        return false;
    }
    g_profilePath = std::move(dir);
    app::setConfigDirOverride(g_profilePath);
    return true;
}

const std::string& isolatedProfilePath() {
    return g_profilePath;
}

void cleanupIsolatedTestProfile() {
    if (g_profilePath.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::remove_all(g_profilePath, ec);
    g_profilePath.clear();
    app::setConfigDirOverride({});
}

} // namespace app::testing
