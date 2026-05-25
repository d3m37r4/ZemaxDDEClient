#pragma once

#include "imgui.h"

namespace ImGuiUtils {
    /// Vertical spacer proportional to current font size.
    /// @param multiplier  Fraction of FontSize (e.g. 0.5f = half line, 1.0f = one line)
    inline void SpacingY(float multiplier = 1.0f) {
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * multiplier));
    }

    /// Horizontal spacer proportional to current font size.
    /// @param multiplier  Fraction of FontSize
    inline void SpacingX(float multiplier = 1.0f) {
        ImGui::Dummy(ImVec2(ImGui::GetFontSize() * multiplier, 0.0f));
    }

    /// Separator with vertical padding proportional to font size.
    /// @param topMultiplier    Vertical spacing before separator (× FontSize)
    /// @param bottomMultiplier Vertical spacing after separator (× FontSize)
    inline void SpacedSeparator(float topMultiplier = 0.5f, float bottomMultiplier = 0.5f) {
        SpacingY(topMultiplier);
        ImGui::Separator();
        SpacingY(bottomMultiplier);
    }

    /// Shows a (?)-mark with a tooltip on hover.
    inline void HelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 45.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    /// Centers the next popup/modal window in the viewport.
    inline void SetPopupWindowPosition() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    /// Applies DPI-aware min-size constraints to the next window.
    inline void SetDpiScaledWindowConstraints(float minWidth, float minHeight) {
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(minWidth * dpiScale, minHeight * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
    }
} // namespace ImGuiUtils
