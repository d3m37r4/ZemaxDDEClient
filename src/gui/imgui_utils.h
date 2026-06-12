#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>
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

    /// Centers the next window in the viewport (call every frame; takes effect on appearing).
    inline void CenterNextWindow() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    /// Begin a two-column property grid (label | value).
    inline void BeginPropertyGrid(const char* id, float labelWidth) {
        ImGui::BeginTable(id, 2);
        ImGui::TableSetupColumn("##L", ImGuiTableColumnFlags_WidthFixed, labelWidth);
        ImGui::TableSetupColumn("##V", ImGuiTableColumnFlags_WidthStretch);
    }

    inline void EndPropertyGrid() {
        ImGui::EndTable();
    }

    inline void PropertyGridRow(const char* label, const char* value) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(value);
    }

    /// Applies DPI-aware min-size constraints to the next window.
    inline void SetDpiScaledWindowConstraints(float minWidth, float minHeight) {
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(minWidth * dpiScale, minHeight * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
    }

    /// Disabled button with animated spinner arc (3/4 circle) inside.
    /// Shows a spinning indicator when @p isActive=true, otherwise a normal Button.
    /// @return true only when clicked in non-active state.
    inline bool SpinnerButton(const char* label, bool isActive,
                              const ImVec2& size = ImVec2(0, 0)) {
        if (!isActive)
            return ImGui::Button(label, size);

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 textSize = ImGui::CalcTextSize(label);

        float spinnerRadius = (textSize.y + style.FramePadding.y * 2.0f) * 0.3f;
        float spinnerDiameter = spinnerRadius * 2.0f;
        float spacing = style.ItemSpacing.x;

        ImVec2 btnSize = size;
        if (btnSize.x == 0.0f) {
            btnSize.x = textSize.x + spinnerDiameter + spacing + style.FramePadding.x * 2.0f;
            btnSize.y = textSize.y + style.FramePadding.y * 2.0f;
        }

        ImGui::PushID(label);
        ImGui::InvisibleButton("##spinner", btnSize);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2& pos = ImGui::GetItemRectMin();

        float disabledAlpha = style.DisabledAlpha;
        dl->AddRectFilled(pos, ImVec2(pos.x + btnSize.x, pos.y + btnSize.y),
                          ImGui::GetColorU32(ImGuiCol_Button, disabledAlpha),
                          style.FrameRounding);

        float angle = ImGui::GetTime() * 5.0f;
        ImVec2 spinnerCenter(pos.x + style.FramePadding.x + spinnerRadius,
                             pos.y + btnSize.y * 0.5f);
        dl->PathArcTo(spinnerCenter, spinnerRadius, angle,
                      angle + static_cast<float>(std::numbers::pi) * 1.5f, 32);
        dl->PathStroke(ImGui::GetColorU32(ImGuiCol_Text, disabledAlpha), 0, 2.0f);

        ImVec2 textPos(pos.x + style.FramePadding.x + spinnerDiameter + spacing,
                       pos.y + (btnSize.y - textSize.y) * 0.5f);
        dl->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text, disabledAlpha), label);

        return false;
    }
} // namespace ImGuiUtils
