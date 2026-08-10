#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace app::services { enum class DisplayUnit : int; }

namespace gui {
    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

    double unitScaleFactor(app::services::DisplayUnit unit);
    const char* displayUnitName(app::services::DisplayUnit unit);
}