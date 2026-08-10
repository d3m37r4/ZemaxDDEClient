#include <fstream>

#include "gui/gui.h"
#include "lib/imgui/imgui.h"
#include "app/services/surface_profile_service.h"

namespace gui {
    const char* getUnitString(int unitCode, bool full) {
        switch (unitCode) {
            case 0: return full ? "Millimeters" : "mm";
            case 1: return full ? "Centimeters" : "cm";
            case 2: return full ? "Inches" : "in";
            case 3: return full ? "Meters" : "m";
            default: return full ? "Unknown unit" : "unknown";
        }
    }

    const char* getRayAimingTypeString(int rayAimingType) {
        switch (rayAimingType) {
            case 0: return "Off";
            case 1: return "Paraxial";
            case 2: return "Real";
            default: return "Unknown";
        }
    }

    double unitScaleFactor(app::services::DisplayUnit unit) {
        return unit == app::services::DisplayUnit::Micrometers ? 1000.0 : 1.0;
    }

    const char* displayUnitName(app::services::DisplayUnit unit) {
        return unit == app::services::DisplayUnit::Micrometers ? "μm" : "mm";
    }

    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content) {
        try {
            const auto tempPath = std::filesystem::temp_directory_path() / filename;
            std::ofstream file(tempPath);

            if (!file.is_open()) {
                return std::nullopt;
            }

            file << content;
            file.close();

            return tempPath;
        } catch (...) {
            return std::nullopt;
        }
    }

}