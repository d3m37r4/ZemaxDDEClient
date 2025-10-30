#include "gui.h"

namespace gui {
    const char* getUnitString(int unitCode) {
        switch (unitCode) {
            case 0: return "mm";
            case 1: return "cm";
            case 2: return "in";
            case 3: return "m";
            default: return "unknown";
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

    void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 45.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}