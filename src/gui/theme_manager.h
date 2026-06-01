#pragma once

#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>
#include <implot3d.h>

struct ThemeData {
    std::string name;
    bool isLight;

    ImVec4 clearColor;

    ImVec4 imguiColors[ImGuiCol_COUNT];
    ImVec4 implotColors[ImPlotCol_COUNT];
    ImVec4 implot3dColors[ImPlot3DCol_COUNT];

    static ThemeData CreateWin11Light();
    static ThemeData CreateWin11Dark();
};

class ThemeManager {
public:
    void registerTheme(const ThemeData& theme);
    bool apply(const std::string& name);
    void toggle();
    void next();

    bool isLight() const;
    ImVec4 getClearColor() const;
    const std::string& currentThemeName() const;
    size_t themeCount() const { return m_themes.size(); }

private:
    struct RegisteredTheme {
        ThemeData data;
    };
    std::vector<RegisteredTheme> m_themes;
    size_t m_current = 0;

    void applyThemeData(const ThemeData& data);
};
