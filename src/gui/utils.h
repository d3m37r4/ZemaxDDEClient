#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace gui {
    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    void HelpMarker(const char* desc);

    // File utility functions
    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);
}
