#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <imgui.h>
#include <implot.h>
#include <implot3d.h>

#include "app/settings.h"

inline constexpr std::string_view kThemeNameLight = "Windows 11 Light";
inline constexpr std::string_view kThemeNameDark  = "Windows 11 Dark";

/// Non-color styling (rounding, padding, spacing, border sizes).
/// Stored per-theme so future themes can override look-and-feel without
/// touching color values. Defaults match the legacy hardcoded applyGeometry().
struct ThemeGeometry {
    float windowRounding    = 8.0f;
    float childRounding     = 8.0f;
    float frameRounding     = 4.0f;
    float popupRounding     = 8.0f;
    float scrollbarRounding = 4.0f;
    float grabRounding      = 4.0f;
    float tabRounding       = 4.0f;

    ImVec2 windowPadding    = ImVec2(12, 12);
    ImVec2 framePadding     = ImVec2(8,  4);
    ImVec2 itemSpacing      = ImVec2(8,  6);
    ImVec2 itemInnerSpacing = ImVec2(8,  6);
    ImVec2 cellPadding      = ImVec2(8,  4);

    float indentSpacing     = 25.0f;
    float scrollbarSize     = 14.0f;
    float grabMinSize       = 12.0f;

    float windowBorderSize  = 1.0f;
    float childBorderSize   = 1.0f;
    float popupBorderSize   = 1.0f;
    float frameBorderSize   = 0.0f;
    float tabBorderSize     = 0.0f;
};

struct ThemeData {
    std::string name;
    bool isLight;

    ImVec4 clearColor;

    ImVec4 imguiColors[ImGuiCol_COUNT];
    ImVec4 implotColors[ImPlotCol_COUNT];
    ImVec4 implot3dColors[ImPlot3DCol_COUNT];

    ThemeGeometry geometry;

    static ThemeData CreateWin11Light();
    static ThemeData CreateWin11Dark();
};

class ThemeManager {
public:
    void registerTheme(const ThemeData& theme);
    bool apply(const std::string& name);
    void toggle();
    void next();

    /// Resolves @p mode against @p isSystemDark and applies the resulting theme.
    /// Used by SettingsManager for AppSettings.appearance.themeMode.
    void applyByMode(app::ThemeMode mode, bool isSystemDark);

    bool isLight() const;
    ImVec4 getClearColor() const;
    const std::string& currentThemeName() const;
    size_t themeCount() const { return m_themes.size(); }

private:
    std::vector<ThemeData> m_themes;
    size_t m_current = 0;

    void applyThemeData(const ThemeData& data);
    void applyGeometryData(const ThemeGeometry& geometry);
};
