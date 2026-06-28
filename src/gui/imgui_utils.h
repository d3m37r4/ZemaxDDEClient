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

    /// Sets DPI-scaled window size for the next window.
    inline void SetDpiScaledWindowSize(const ImVec2& size, ImGuiCond cond = ImGuiCond_Once) {
        float dpi = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSize(ImVec2(size.x * dpi, size.y * dpi), cond);
    }

    /// Scales a value by the current window DPI factor.
    inline float DpiScale(float value) {
        return value * ImGui::GetWindowDpiScale();
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
        dl->PathStroke(ImGui::GetColorU32(ImGuiCol_Text, disabledAlpha), 0, DpiScale(2.0f));

        ImVec2 textPos(pos.x + style.FramePadding.x + spinnerDiameter + spacing,
                       pos.y + (btnSize.y - textSize.y) * 0.5f);
        dl->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text, disabledAlpha), label);

        return false;
    }

    /// Horizontal splitter that resizes left/right panels.
    /// Call between two SameLine() pairs.
    /// @param id       Unique ID (e.g. "##splitter_1")
    /// @param value    Current left-panel width (modified by drag)
    /// @param height   Splitter height
    /// @param width    Splitter bar thickness (default 4.0f)
    /// @param minVal   Minimum value clamp
    /// @param maxVal   Maximum value clamp
    /// @return true if value changed
    inline bool SplitterH(const char* id, float& value, float height,
                           float width = 4.0f, float minVal = 0.0f, float maxVal = FLT_MAX) {
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(width, height));

        if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        bool changed = false;
        if (ImGui::IsItemActive()) {
            value += ImGui::GetIO().MouseDelta.x;
            value = std::clamp(value, minVal, maxVal);
            changed = true;
        }

        ImU32 col;
        if (ImGui::IsItemActive())
            col = ImGui::GetColorU32(ImGuiCol_SeparatorActive);
        else if (ImGui::IsItemHovered())
            col = ImGui::GetColorU32(ImGuiCol_Text, 0.4f);
        else
            col = ImGui::GetColorU32(ImGuiCol_Separator);

        ImGui::GetWindowDrawList()->AddRectFilled(sp, ImVec2(sp.x + width, sp.y + height), col);

        return changed;
    }
    /// Renders a section header with title, separator, and optional description.
    inline void SectionHeader(const char* title, const char* description = nullptr) {
        ImGui::TextUnformatted(title);
        ImGui::Separator();
        ImGui::Spacing();
        if (description) {
            ImGui::TextDisabled(description);
            ImGui::Spacing();
        }
    }

} // namespace ImGuiUtils
