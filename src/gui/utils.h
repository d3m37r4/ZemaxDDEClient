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

    /// Number with thousand separators: "12 060 000" reads better than
    /// "12060000". Separators are placed right to left, so the index
    /// decreases by three after each insertion.
    inline std::string formatCount(long long value) {
        const bool negative = value < 0;
        std::string digits = std::to_string(negative ? -value : value);
        for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) {
            digits.insert(static_cast<size_t>(i), " ");
        }
        return (negative ? "-" : "") + digits;
    }
}