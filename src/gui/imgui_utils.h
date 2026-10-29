#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>
#include "imgui.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"

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
        ImGui::TextDisabled("%s", ICON_FA_CIRCLE_QUESTION);
        if (ImGui::BeginItemTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 45.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    /// Centers the next window in the viewport (call every frame; takes effect
    /// on appearing). A viewport resize (maximize/restore, monitor change)
    /// while the window is already open forces an immediate recenter in that
    /// frame: Appearing-only centering leaves an open dialog stranded.
    ///
    /// The resize detection is resolved once per frame: this helper is called
    /// several times in a frame (one per popup), and the first caller would
    /// otherwise refresh the cache leaving nothing for the actually-open
    /// dialog rendered later in the same frame.
    inline void CenterNextWindow() {
        static int lastDetectionFrame = -1;
        static ImVec2 lastViewportSize(-1.0f, -1.0f);
        static bool viewportResizedThisFrame = false;
        const int frame = ImGui::GetFrameCount();
        if (frame != lastDetectionFrame) {
            const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
            viewportResizedThisFrame = lastViewportSize.x != viewportSize.x ||
                                       lastViewportSize.y != viewportSize.y;
            lastViewportSize = viewportSize;
            lastDetectionFrame = frame;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
            viewportResizedThisFrame ? ImGuiCond_Always : ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f));
    }

    /// Keeps a floating window fully inside its viewport: after a main window
    /// shrink (or a stale ini position), an undocked window keeps its absolute
    /// position and may end up out of sight. Docked windows live inside a
    /// dock node and are always visible - never move those. Call right after
    /// Begin() returned true. The clamp region is the window's OWN viewport,
    /// which under multi-viewport is the bounds of its platform (OS) window
    /// and otherwise the main viewport - one formula covers both. Windows
    /// with a forced chrome rect (maximized floating panels) pass skip=true:
    /// their geometry is managed by window_chrome and spans exactly.
    inline void ClampWindowToViewport(bool skip = false) {
        if (skip || ImGui::IsWindowDocked()) return;
        const ImVec2 vpPos = ImGui::GetWindowViewport()->Pos;
        const ImVec2 vpSize = ImGui::GetWindowViewport()->Size;
        const ImVec2 size = ImGui::GetWindowSize();
        const ImVec2 pos = ImGui::GetWindowPos();
        // Fully-inside clamp; a window wider/taller than the viewport anchors
        // to the corresponding edge.
        ImVec2 target = pos;
        target.x = (size.x >= vpSize.x) ? vpPos.x
            : std::min(std::max(pos.x, vpPos.x), vpPos.x + vpSize.x - size.x);
        target.y = (size.y >= vpSize.y) ? vpPos.y
            : std::min(std::max(pos.y, vpPos.y), vpPos.y + vpSize.y - size.y);
        ImGui::SetWindowPos(target, ImGuiCond_Always);
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

    /// Width of the title-bar chrome strip (menu, minimize, maximize and
    /// close squares with pitch), computed from the live style - used as a
    /// floor so any theme/DPI keeps the text clear of the buttons
    /// (DESIGN.md rules).
    inline float ChromeTitleBarStripWidth() {
        const ImGuiStyle& s = ImGui::GetStyle();
        const float b = ImGui::GetFontSize();
        return s.FramePadding.x * 2.0f     // edge pads
            + 4.0f * b                     // menu, −, □, ✕ squares
            + 3.0f * s.ItemInnerSpacing.x; // button pitch
    }

    /// Applies DPI-aware min-size constraints to the next window.
    inline void SetDpiScaledWindowConstraints(float minWidth, float minHeight) {
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(minWidth * dpiScale, minHeight * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
    }

    /// Min-size constraint for chrome windows:
    /// width = max(per-window content constant, title-bar strip).
    inline void SetChromeWindowConstraints(float minWidth, float minHeight) {
        SetDpiScaledWindowConstraints(
            std::max(minWidth, ChromeTitleBarStripWidth()), minHeight);
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

    /// Scales an ImVec2 (e.g. a size or position) by the current window DPI factor.
    inline ImVec2 DpiScaleVec2(const ImVec2& value) {
        return ImVec2(value.x * ImGui::GetWindowDpiScale(),
                      value.y * ImGui::GetWindowDpiScale());
    }

    /// Low-level 270-degree arc spinner: draws a background ring plus a rotating
    /// arc at an absolute position (no layout). Shared by BusyArc and SpinnerButton.
    /// @param speed  Revolutions scaled by time (6.0 matches the approved look).
    inline void DrawBusyArc(const ImVec2& center, float radius, const ImVec4& color,
                            float speed = 6.0f) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // The stroke scales with the radius, so that the small spinner in a
        // button and the large one in the status bar carry the same weight.
        // The radius is already in device pixels.
        const float thickness = std::clamp(radius * 0.22f, 1.5f, 3.0f);
        dl->AddCircle(center, radius,
            ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * 0.5f)), 32, thickness);
        const float a0 = static_cast<float>(ImGui::GetTime()) * speed;
        dl->PathArcTo(center, radius, a0, a0 + 1.5f * std::numbers::pi_v<float>, 32);
        // StrokeInside puts the arc on the same circle as the background ring:
        // paths stroke on the path center by default, closed shapes inside.
        dl->PathStroke(ImGui::GetColorU32(color), thickness, ImDrawFlags_StrokeInside);
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
        const ImVec2 btnMax(pos.x + btnSize.x, pos.y + btnSize.y);
        // A disabled button is drawn by hand, so it needs a fill of its own:
        // the light theme's button fill is white and would dissolve into the
        // panel. The inactive tab tone is the muted neutral of both themes.
        dl->AddRectFilled(pos, btnMax,
                          ImGui::GetColorU32(ImGuiCol_Tab),
                          style.FrameRounding);
        // The frame the theme puts on its widgets: without it the button has
        // no edge at all.
        if (style.FrameBorderSize > 0.0f) {
            dl->AddRect(pos, btnMax, ImGui::GetColorU32(ImGuiCol_Border),
                        style.FrameRounding, style.FrameBorderSize);
        }

        // Legacy arc (replaced by DrawBusyArc above, kept for reference):
        // float angle = ImGui::GetTime() * 5.0f;
        // ImVec2 spinnerCenter(pos.x + style.FramePadding.x + spinnerRadius,
        //                      pos.y + btnSize.y * 0.5f);
        // dl->PathArcTo(spinnerCenter, spinnerRadius, angle,
        //               angle + static_cast<float>(std::numbers::pi) * 1.5f, 32);
        // dl->PathStroke(ImGui::GetColorU32(ImGuiCol_Text, disabledAlpha), 0, DpiScale(2.0f));
        ImVec4 spinCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        spinCol.w *= disabledAlpha;
        DrawBusyArc(ImVec2(pos.x + style.FramePadding.x + spinnerRadius,
                           pos.y + btnSize.y * 0.5f),
                    spinnerRadius, spinCol);

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
    /// Renders a section header with separator title + separator line, optional description.
    inline void SectionHeader(const char* title, const char* description = nullptr) {
        ImGui::SeparatorText(title);
        if (description) {
            ImGui::TextDisabled(description);
            ImGui::Spacing();
        }
    }

    /// Pushes a danger-tinted FrameBg while the field holds a value that was
    /// clamped/snapped on the last edit. Returns true when pushed — the caller
    /// must call ImGui::PopStyleColor() after the input. Colors come from
    /// SemanticPalette — never hardcode.
    inline bool BeginAdjustedHighlight(bool adjusted, const ImVec4& danger) {
        if (!adjusted) return false;
        ImVec4 tint = danger;
        tint.w = 0.28f;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, tint);
        return true;
    }

    /// Warning hint under an input whose value was clamped/snapped.
    /// No-op when @p adjusted is false. Call after the input row.
    inline void AdjustedHint(bool adjusted, const char* text, const ImVec4& warning) {
        if (!adjusted) return;
        ImGui::TextColored(warning, "%s", text);
    }

    /// 270-degree arc spinner (custom draw, no third-party dependency).
    /// Reserves a full row height (like BusyEqualizer) so the arc centers on
    /// tall neighbours such as combo boxes instead of sticking to the top.
    /// @param diameterPx  Final pixel diameter (already DPI-aware).
    /// @param color       Theme color — never hardcode.
    /// @param rowHeightPx Full row box height; 0 keeps the legacy compact box.
    inline void BusyArc(const char* id, float diameterPx, const ImVec4& color, float rowHeightPx = 0.0f) {
        (void)id;
        const float r = diameterPx * 0.5f;
        const float boxH = rowHeightPx > diameterPx ? rowHeightPx : diameterPx;
        ImGui::Dummy(ImVec2(diameterPx, boxH));
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mx = ImGui::GetItemRectMax();
        DrawBusyArc(ImVec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f), r, color);
    }

    /// Rectangular busy indicator: three fading bars strictly inside their
    /// layout box (the vendored FadeBars overpaints neighbouring items, so it
    /// is unsuitable for dense rows). Reserves exactly one text-line height —
    /// the same box Selectable uses — so bars center on the text by construction.
    /// Fits combo items.
    /// @param widthPx   Final pixel width of the whole widget (DPI-aware).
    /// @param heightPx  Final pixel bar-area height (DPI-aware, e.g. text height).
    /// @param color     Theme color — never hardcode.
    inline void BusyEqualizer(float widthPx, float heightPx, const ImVec4& color) {
        const float lineH = ImGui::GetTextLineHeight();
        const float h = std::min(heightPx, lineH);
        ImGui::Dummy(ImVec2(widthPx, lineH));
        const ImVec2 mn = ImGui::GetItemRectMin();
        const ImVec2 mx = ImGui::GetItemRectMax();
        const float yc = (mn.y + mx.y) * 0.5f;
        const float y0 = yc - h * 0.5f;
        const float y1 = yc + h * 0.5f;
        const float t = static_cast<float>(ImGui::GetTime()) * 2.8f;
        const float bw = (mx.x - mn.x) / 3.0f;
        for (int i = 0; i < 3; ++i) {
            const float frac = 0.35f + 0.65f * (0.5f + 0.5f * sinf(t - i * std::numbers::pi_v<float> / 3.0f));
            const float bh = h * frac;
            const float bc = (y0 + y1) * 0.5f;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(mn.x + bw * i + 1.0f, bc - bh * 0.5f),
                ImVec2(mn.x + bw * (i + 1) - 1.0f, bc + bh * 0.5f),
                ImGui::GetColorU32(ImVec4(color.x, color.y, color.z,
                    color.w * (0.35f + 0.65f * frac))));
        }
    }

    /// Width of an icon-only CopyButton (for SetNextItemWidth math at call sites).
    inline float CopyButtonWidth() {
        return ImGui::CalcTextSize(ICON_FA_COPY).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    }

    /// Icon-only copy button. Uses the theme's neutral button color (the
    /// "control" step of the surface ladder), so it matches every other
    /// neutral button. One click instead of select + Ctrl+C.
    /// Disabled when text is empty.
    /// @param labelWithId  Visible icon + unique ID, e.g. ICON_FA_COPY "##copy_x".
    /// @return true when clicked.
    inline bool CopyButton(const char* labelWithId, const char* text,
                           const char* tooltip = "Copy to clipboard") {
        ImGui::BeginDisabled(!text || !*text);
        const bool clicked = ImGui::Button(labelWithId);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", tooltip);
        if (clicked && text && *text)
            ImGui::SetClipboardText(text);
        return clicked;
    }

    /// Pushes the style of a filled (status) button: the fill, its hover and
    /// active shades, the text color, and a border in the fill color.
    ///
    /// The border is what keeps a light theme whose widgets carry a hairline
    /// frame (the input fields) from putting a grey ring around a colored
    /// button.
    inline void PushFilledButton(const ImVec4& fill, const ImVec4& hover,
                                 const ImVec4& active, const ImVec4& text) {
        ImGui::PushStyleColor(ImGuiCol_Button, fill);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
        ImGui::PushStyleColor(ImGuiCol_Border, fill);
        ImGui::PushStyleColor(ImGuiCol_Text, text);
    }

    /// Pops what PushFilledButton() pushed.
    inline void PopFilledButton() {
        ImGui::PopStyleColor(5);
    }

    /// True while a value clamped at @p clampedAt (ImGui::GetTime()) should keep
    /// its "adjusted" flash. Self-expiring — no flag to clear, no stale states.
    inline bool AdjustedFlashActive(double clampedAt, float windowSec = 3.0f) {
        if (clampedAt <= 0.0) return false;
        return (ImGui::GetTime() - clampedAt) < static_cast<double>(windowSec);
    }

    /// BeginPopupModal with a taller title bar (extraPadY added to FramePadding.y).
    /// PopStyleVar is called immediately after BeginPopupModal so content is unaffected.
    inline bool BeginPopupModalEx(const char* name, bool* p_open = nullptr,
        ImGuiWindowFlags flags = 0, float extraPadY = 4.0f)
    {
        ImVec2 pad = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad.x, pad.y + extraPadY));
        bool open = ImGui::BeginPopupModal(name, p_open, flags);
        ImGui::PopStyleVar();
        return open;
    }

} // namespace ImGuiUtils
