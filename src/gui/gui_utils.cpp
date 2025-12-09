#include <fstream>

#include "gui.h"

namespace gui {
    void renderPageHeader(GuiPage currentPage) {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), GUI_PAGES[static_cast<size_t>(currentPage)].title);
        ImGui::Spacing();
    }

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

    void HelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");

        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 45.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
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