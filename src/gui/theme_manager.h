#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <imgui.h>
#include <implot.h>
#include <implot3d.h>

#include "app/settings.h"

inline constexpr std::string_view kThemeNameLight = "Light Theme";
inline constexpr std::string_view kThemeNameDark  = "Dark Theme";

/// Non-color styling (rounding, padding, spacing, border sizes).
/// Stored per-theme so future themes can override look-and-feel without
/// touching color values. Defaults match the legacy hardcoded applyGeometry().
struct ThemeGeometry {
    float windowRounding    = 5.0f;
    float childRounding     = 5.0f;
    float frameRounding     = 5.0f;
    float popupRounding     = 5.0f;
    float scrollbarRounding = 3.0f;
    float grabRounding      = 3.0f;
    float tabRounding       = 5.0f;

    ImVec2 windowPadding    = ImVec2(12, 12);
    ImVec2 framePadding     = ImVec2(8,  4);
    ImVec2 itemSpacing      = ImVec2(8,  6);
    ImVec2 itemInnerSpacing = ImVec2(8,  6);
    ImVec2 cellPadding      = ImVec2(8,  4);

    float indentSpacing     = 25.0f;
    float scrollbarSize     = 10.0f;
    float grabMinSize       = 12.0f;

    float windowBorderSize  = 1.0f;
    float childBorderSize   = 1.0f;
    float popupBorderSize   = 1.0f;
    float frameBorderSize   = 0.0f;
    float tabBorderSize     = 0.0f;
};

/// Semantic, role-based color tokens used by widgets that need to communicate
/// status (success/danger/info/warning/muted) or theme-aware button shades.
/// Replaces the legacy hardcoded DDE_STATUS_COLOR_* and DDE_BUTTON_* constants
/// from gui/constants.h so each theme can provide its own status palette.
struct SemanticPalette {
    ImVec4 success;          // Connected, "new version available", check marks.
    ImVec4 warning;          // Validation warning, prerelease notice.
    ImVec4 danger;           // Disconnected, errors, destructive actions.
    ImVec4 info;             // Hints, links (usually matches theme accent).
    ImVec4 muted;            // "Up to date", neutral captions.

    ImVec4 successButton;
    ImVec4 successButtonHover;
    ImVec4 successButtonActive;

    ImVec4 dangerButton;
    ImVec4 dangerButtonHover;
    ImVec4 dangerButtonActive;

    ImVec4 neutralButton;
    ImVec4 neutralButtonHover;
    ImVec4 neutralButtonActive;

    ImVec4 onAccent;         // Text drawn on top of an accent-colored button.
};

struct ThemeData {
    std::string name;
    bool isLight;

    ImVec4 clearColor;

    ImVec4 imguiColors[ImGuiCol_COUNT];
    ImVec4 implotColors[ImPlotCol_COUNT];
    ImVec4 implot3dColors[ImPlot3DCol_COUNT];

    ThemeGeometry geometry;
    SemanticPalette semantic;

    static ThemeData CreateThemeLight();
    static ThemeData CreateThemeDark();
};

class ThemeManager {
public:
    void registerTheme(const ThemeData& theme);
    bool apply(const std::string& name);

    /// Resolves @p mode against @p isSystemDark and applies the resulting theme.
    /// Used by SettingsManager for AppSettings.appearance.themeMode.
    void applyByMode(app::ThemeMode mode, bool isSystemDark);

    /// Sets the current window DPI scale factor used to scale geometry tokens
    /// (rounding, padding, spacing, border sizes) when applying a theme.
    /// @note ImGui::GetWindowDpiScale() is unreliable before the first frame,
    /// so the caller (GraphicsBackend) passes the value from
    /// glfwGetWindowContentScale() instead.
    void setDpiScale(float dpiScale);

    bool isLight() const;
    ImVec4 getClearColor() const;
    const std::string& currentThemeName() const;

    /// Returns the semantic palette of the currently active theme.
    /// Widgets that draw status text or status-colored buttons should read
    /// from here so colors stay consistent with the active palette.
    const SemanticPalette& semantic() const;

private:
    std::vector<ThemeData> m_themes;
    size_t m_current = 0;
    float m_dpiScale = 1.0f;

    void applyThemeData(const ThemeData& data);
    void applyGeometryData(const ThemeGeometry& geometry);
};
