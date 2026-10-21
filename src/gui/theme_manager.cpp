#include "gui/theme_manager.h"
#include "gui/constants.h"

#include <algorithm>
#include <cmath>

// Helper: create an ImVec4 from 0..255 RGBA bytes
static ImVec4 col(int r, int g, int b, int a = 255) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

// Light Theme
ThemeData ThemeData::CreateThemeLight() {
    ThemeData t;
    t.name    = std::string{kThemeNameLight};
    t.isLight = true;
    t.clearColor = col(232, 234, 237);  // #E8EAED (canvas)

    // The single accent color (dark counterpart of the cyan accent).
    constexpr int kAccentR = 27, kAccentG = 94, kAccentB = 158;   // #1b5e9e
    auto accent = [=](int alpha = 255) { return col(kAccentR, kAccentG, kAccentB, alpha); };
    auto accentLight = [=](int alpha = 255) { return col(46, 121, 190, alpha); };  // #2e79be

    // Semantic palette
    t.semantic.success = col(53, 112, 58);        // #35703a
    t.semantic.warning = col(138, 90, 0);         // #8a5a00
    t.semantic.danger  = col(179, 57, 46);        // #b3392e
    t.semantic.info    = accent();                 // #0e6e75 (accent)
    t.semantic.muted   = col(95, 102, 112);       // #5f6670

    t.semantic.successButton       = col(53, 112, 58);
    t.semantic.successButtonHover  = col(47, 102, 52);
    t.semantic.successButtonActive = col(39, 86, 43);
    t.semantic.dangerButton        = col(179, 57, 46);
    t.semantic.dangerButtonHover   = col(192, 57, 43);
    t.semantic.dangerButtonActive  = col(153, 47, 38);
    t.semantic.neutralButton       = col(241, 242, 244); // #F1F2F4
    t.semantic.neutralButtonHover  = col(227, 229, 233); // #E3E5E9
    t.semantic.neutralButtonActive = col(216, 219, 224); // #D8DBE0
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    // Surface layering (see DESIGN.md):
    //   canvas #E8EAED -> card #FFFFFF -> element #F1F2F4 -> hover #E3E5E9
    c[ImGuiCol_Text]                 = col(36,  39,  44);   // #24272c
    c[ImGuiCol_TextDisabled]         = col(95, 102, 112);  // #5f6670

    c[ImGuiCol_WindowBg]             = col(232, 234, 237);  // #e8eaed canvas
    c[ImGuiCol_ChildBg]              = col(255, 255, 255);  // #ffffff card
    c[ImGuiCol_PopupBg]              = col(255, 255, 255, 250);
    c[ImGuiCol_MenuBarBg]            = col(232, 234, 237);

    c[ImGuiCol_Border]               = col(210, 214, 219);  // #d2d6db hairline
    c[ImGuiCol_BorderShadow]         = col(0,   0,   0,   15);

    c[ImGuiCol_FrameBg]              = col(241, 242, 244);  // #f1f2f4 element
    c[ImGuiCol_FrameBgHovered]       = col(227, 229, 233);  // #e3e5e9
    c[ImGuiCol_FrameBgActive]        = col(216, 219, 224);  // #d8dbe0

    c[ImGuiCol_TitleBg]              = col(232, 234, 237);  // tab strip = canvas
    c[ImGuiCol_TitleBgActive]        = col(232, 234, 237);
    c[ImGuiCol_TitleBgCollapsed]     = col(222, 224, 228);  // #dee0e4

    c[ImGuiCol_ScrollbarBg]          = col(232, 234, 237);
    c[ImGuiCol_ScrollbarGrab]        = col(193, 198, 204);
    c[ImGuiCol_ScrollbarGrabHovered] = col(156, 163, 175);
    c[ImGuiCol_ScrollbarGrabActive]  = col(107, 114, 128);

    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accentLight();
    c[ImGuiCol_InputTextCursor]      = accent();

    c[ImGuiCol_Button]               = col(241, 242, 244);  // #f1f2f4
    c[ImGuiCol_ButtonHovered]        = col(227, 229, 233);  // #e3e5e9
    c[ImGuiCol_ButtonActive]         = col(216, 219, 224);  // #d8dbe0

    c[ImGuiCol_Header]               = col(227, 229, 233);  // #e3e5e9
    c[ImGuiCol_HeaderHovered]        = accent(40);   // accent @16%
    c[ImGuiCol_HeaderActive]         = accent(80);   // accent @31%
    c[ImGuiCol_Separator]            = col(223, 226, 230);  // #dfe2e6
    c[ImGuiCol_SeparatorHovered]     = accent();
    c[ImGuiCol_SeparatorActive]      = accent();

    c[ImGuiCol_ResizeGrip]           = accent(40);
    c[ImGuiCol_ResizeGripHovered]    = accent(110);
    c[ImGuiCol_ResizeGripActive]     = accent(170);

    c[ImGuiCol_Tab]                  = col(241, 242, 244);  // #f1f2f4
    c[ImGuiCol_TabHovered]           = col(227, 229, 233);  // #e3e5e9
    c[ImGuiCol_TabSelected]          = col(255, 255, 255);  // #ffffff card
    c[ImGuiCol_TabSelectedOverline]  = accent();   // accent underline
    c[ImGuiCol_TabDimmed]            = col(241, 242, 244);
    c[ImGuiCol_TabDimmedSelected]    = col(255, 255, 255);
    c[ImGuiCol_TabDimmedSelectedOverline] = col(95, 102, 112);

    c[ImGuiCol_DockingPreview]       = accent(100);
    c[ImGuiCol_DockingEmptyBg]       = col(241, 242, 244);  // #f1f2f4
    c[ImGuiCol_DragDropTarget]       = accent();
    c[ImGuiCol_DragDropTargetBg]     = accent(50);

    c[ImGuiCol_PlotLines]            = accent();
    c[ImGuiCol_PlotLinesHovered]     = accentLight();
    c[ImGuiCol_PlotHistogram]        = accent(180);
    c[ImGuiCol_PlotHistogramHovered] = accentLight(220);
    c[ImGuiCol_TableHeaderBg]        = col(238, 240, 242);  // #eef0f2
    c[ImGuiCol_TableBorderStrong]    = col(210, 214, 219);  // #d2d6db
    c[ImGuiCol_TableBorderLight]     = col(228, 230, 233);
    c[ImGuiCol_TableRowBg]           = col(255, 255, 255);
    c[ImGuiCol_TableRowBgAlt]        = col(246, 247, 248);

    c[ImGuiCol_TextLink]             = accent();
    c[ImGuiCol_TextSelectedBg]       = accent(64);
    c[ImGuiCol_TreeLines]            = col(210, 214, 219);  // #d2d6db
    c[ImGuiCol_UnsavedMarker]        = accent();
    c[ImGuiCol_NavCursor]            = accent();
    c[ImGuiCol_NavWindowingHighlight]= col(255, 255, 255);
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   100);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   130);

    // ImPlot
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(255, 255, 255);
    pc[ImPlotCol_PlotBorder]    = col(210, 214, 219);  // #d2d6db
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = col(210, 214, 219);  // #d2d6db
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(36, 39, 44, 28);
    pc[ImPlotCol_AxisTick]      = col(36, 39, 44, 60);
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = accent(60);
    pc[ImPlotCol_Crosshairs]    = col(107, 114, 128, 150);

    // ImPlot3D
    ImVec4* p3 = t.implot3dColors;
    p3[ImPlot3DCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_InlayText]     = pc[ImPlotCol_InlayText];
    p3[ImPlot3DCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    p3[ImPlot3DCol_PlotBg]        = pc[ImPlotCol_PlotBg];
    p3[ImPlot3DCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    p3[ImPlot3DCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    p3[ImPlot3DCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    p3[ImPlot3DCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_AxisGrid]      = pc[ImPlotCol_AxisGrid];
    p3[ImPlot3DCol_AxisTick]      = pc[ImPlotCol_AxisTick];
    p3[ImPlot3DCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    p3[ImPlot3DCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    p3[ImPlot3DCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];

    // Rounding of the reference: panels 8px, elements 4px.
    t.geometry.windowRounding = 8.0f;
    t.geometry.childRounding  = 8.0f;
    t.geometry.popupRounding  = 8.0f;
    t.geometry.frameRounding  = 4.0f;
    t.geometry.tabRounding    = 4.0f;

    return t;
}

// Dark Theme
ThemeData ThemeData::CreateThemeDark() {
    ThemeData t;
    t.name    = std::string{kThemeNameDark};
    t.isLight = false;
    t.clearColor = col(15, 15, 15);  // #0F0F0F (canvas)

    // The single accent color. Cyan reads as "instrument" and stays clearly
    // apart from the semantic green (success), amber (warning) and red
    // (danger). Change these three numbers to re-tint the whole theme.
    constexpr int kAccentR = 66, kAccentG = 150, kAccentB = 250;   // #4296fa
    auto accent = [=](int alpha = 255) { return col(kAccentR, kAccentG, kAccentB, alpha); };
    auto accentLight = [=](int alpha = 255) { return col(102, 168, 252, alpha); };  // #66a8fc

    // Semantic palette
    t.semantic.success = col(118, 187, 117);      // #76bb75 (success green)
    t.semantic.warning = col(201, 162, 39);       // #c9a227
    t.semantic.danger  = col(232, 92, 88);        // #e85c58
    t.semantic.info    = accent();                 // #4fc3c7 (accent)
    t.semantic.muted   = col(134, 139, 155);      // #868b9b

    t.semantic.successButton       = col(53, 112, 58);   // #35703a
    t.semantic.successButtonHover  = col(62, 122, 66);   // #3e7a42
    t.semantic.successButtonActive = col(44, 99, 49);    // #2c6331
    t.semantic.dangerButton        = col(179, 57, 46);   // #b3392e
    t.semantic.dangerButtonHover   = col(192, 57, 43);   // #c0392b
    t.semantic.dangerButtonActive  = col(153, 47, 38);   // #992f26
    t.semantic.neutralButton       = col(22, 23, 25);    // #161719
    t.semantic.neutralButtonHover  = col(31, 33, 38);    // #1f2126
    t.semantic.neutralButtonActive = col(38, 42, 49);    // #262a31
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    // Surface layering (see DESIGN.md):
    //   canvas #0F0F0F -> card #141517 -> element #161719 -> hover #1F2126
    c[ImGuiCol_Text]                 = col(201, 204, 214); // #c9ccd6
    c[ImGuiCol_TextDisabled]         = col(134, 139, 155); // #868b9b
    c[ImGuiCol_WindowBg]             = col(15,  15,  15);   // #0f0f0f canvas
    c[ImGuiCol_ChildBg]              = col(20,  21,  23);   // #141517 card
    c[ImGuiCol_PopupBg]              = col(20,  21,  23, 250);
    c[ImGuiCol_MenuBarBg]            = col(15,  15,  15);
    c[ImGuiCol_Border]               = col(38,  42,  49);   // #262a31 hairline
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(22,  23,  25);   // #161719 element
    c[ImGuiCol_FrameBgHovered]       = col(31,  33,  38);   // #1f2126
    c[ImGuiCol_FrameBgActive]        = col(38,  42,  49);   // #262a31
    c[ImGuiCol_TitleBg]              = col(15,  15,  15);   // tab strip = canvas
    c[ImGuiCol_TitleBgActive]        = col(15,  15,  15);
    c[ImGuiCol_TitleBgCollapsed]     = col(10,  10,  10);   // #0a0a0a
    c[ImGuiCol_MenuBarBg]            = col(15,  15,  15);
    c[ImGuiCol_ScrollbarBg]          = col(15,  15,  15);
    c[ImGuiCol_ScrollbarGrab]        = col(42,  45,  52);   // #2a2d34
    c[ImGuiCol_ScrollbarGrabHovered] = col(58,  62,  71);   // #3a3e47
    c[ImGuiCol_ScrollbarGrabActive]  = col(74,  79,  90);   // #4a4f5a
    // Accent: cyan (single accent, distinct from success green).
    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accentLight();
    c[ImGuiCol_InputTextCursor]      = accent();
    c[ImGuiCol_Button]               = col(22,  23,  25);   // #161719
    c[ImGuiCol_ButtonHovered]        = col(31,  33,  38);   // #1f2126
    c[ImGuiCol_ButtonActive]         = col(38,  42,  49);   // #262a31
    c[ImGuiCol_Header]               = col(31,  33,  38);   // #1f2126
    c[ImGuiCol_HeaderHovered]        = accent(40);   // accent @16%
    c[ImGuiCol_HeaderActive]         = accent(80);   // accent @31%
    c[ImGuiCol_Separator]            = col(28,  31,  37);   // #1c1f25
    c[ImGuiCol_SeparatorHovered]     = accent();
    c[ImGuiCol_SeparatorActive]      = accent();
    c[ImGuiCol_ResizeGrip]           = accent(40);
    c[ImGuiCol_ResizeGripHovered]    = accent(110);
    c[ImGuiCol_ResizeGripActive]     = accent(170);
    c[ImGuiCol_TabHovered]           = col(31,  33,  38);   // #1f2126
    c[ImGuiCol_Tab]                  = col(20,  21,  23);   // #141517
    c[ImGuiCol_TabSelected]          = col(31,  33,  38);   // #1f2126
    c[ImGuiCol_TabSelectedOverline]  = accent();       // accent underline
    c[ImGuiCol_TabDimmed]            = col(20,  21,  23);
    c[ImGuiCol_TabDimmedSelected]    = col(31,  33,  38);
    c[ImGuiCol_TabDimmedSelectedOverline] = col(134, 139, 155);
    c[ImGuiCol_DockingPreview]       = accent(90);
    c[ImGuiCol_DockingEmptyBg]       = col(20,  21,  23);   // #141517
    c[ImGuiCol_PlotLines]            = col(155, 160, 172); // #9ba0ac
    c[ImGuiCol_PlotLinesHovered]     = accentLight();
    c[ImGuiCol_PlotHistogram]        = accent(180);
    c[ImGuiCol_PlotHistogramHovered] = accentLight(220);
    c[ImGuiCol_TableHeaderBg]        = col(24,  26,  30);   // #181a1e
    c[ImGuiCol_TableBorderStrong]    = col(48,  52,  60);   // #30343c
    c[ImGuiCol_TableBorderLight]     = col(30,  33,  38);   // #1e2126
    c[ImGuiCol_TableRowBg]           = col(20,  21,  23);
    c[ImGuiCol_TableRowBgAlt]        = col(24,  26,  30);
    c[ImGuiCol_TextLink]             = accent();
    c[ImGuiCol_TextSelectedBg]       = accent(80);
    c[ImGuiCol_TreeLines]            = col(38,  42,  49);   // #262a31
    c[ImGuiCol_DragDropTarget]       = accent();
    c[ImGuiCol_DragDropTargetBg]     = accent(32);
    c[ImGuiCol_UnsavedMarker]        = accent();
    c[ImGuiCol_NavCursor]            = accent();
    c[ImGuiCol_NavWindowingHighlight]= col(31,  33,  38);
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   115);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   153);

    // ImPlot
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(15, 15, 15);
    pc[ImPlotCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(201, 204, 214, 28);
    pc[ImPlotCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = accent(60);
    pc[ImPlotCol_Crosshairs]    = pc[ImPlotCol_PlotBorder];

    // ImPlot3D
    ImVec4* p3 = t.implot3dColors;
    p3[ImPlot3DCol_TitleText]  = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_InlayText]  = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_FrameBg]    = t.imguiColors[ImGuiCol_FrameBg];
    p3[ImPlot3DCol_PlotBg]     = pc[ImPlotCol_PlotBg];
    p3[ImPlot3DCol_PlotBorder] = t.imguiColors[ImGuiCol_Border];
    p3[ImPlot3DCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    p3[ImPlot3DCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    p3[ImPlot3DCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    p3[ImPlot3DCol_AxisGrid]      = pc[ImPlotCol_AxisGrid];
    p3[ImPlot3DCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    p3[ImPlot3DCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    p3[ImPlot3DCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    p3[ImPlot3DCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];

    // Rounding of the reference: panels 8px, elements 4px.
    t.geometry.windowRounding = 8.0f;
    t.geometry.childRounding  = 8.0f;
    t.geometry.popupRounding  = 8.0f;
    t.geometry.frameRounding  = 4.0f;
    t.geometry.tabRounding    = 4.0f;

    return t;
}

// ThemeManager methods
void ThemeManager::registerTheme(const ThemeData& theme) {
    m_themes.push_back(theme);
}

bool ThemeManager::apply(const std::string& name) {
    for (size_t i = 0; i < m_themes.size(); ++i) {
        if (m_themes[i].name == name) {
            m_current = i;
            applyThemeData(m_themes[i]);
            return true;
        }
    }
    return false;
}

void ThemeManager::applyByMode(app::ThemeMode mode, bool isSystemDark) {
    switch (mode) {
        case app::ThemeMode::Light:
            apply(std::string{kThemeNameLight});
            break;
        case app::ThemeMode::Dark:
            apply(std::string{kThemeNameDark});
            break;
        case app::ThemeMode::System:
            apply(std::string{isSystemDark ? kThemeNameDark : kThemeNameLight});
            break;
    }
}

void ThemeManager::setDpiScale(float dpiScale) {
    if (!std::isfinite(dpiScale) || dpiScale <= 0.0f) dpiScale = gui::MIN_DPI_SCALE;
    m_dpiScale = std::clamp(dpiScale, gui::MIN_DPI_SCALE, gui::MAX_DPI_SCALE);
}

bool ThemeManager::isLight() const {
    if (m_themes.empty()) return false;
    return m_themes[m_current].isLight;
}

ImVec4 ThemeManager::getClearColor() const {
    if (m_themes.empty()) return ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    return m_themes[m_current].clearColor;
}

const std::string& ThemeManager::currentThemeName() const {
    static const std::string s_empty;
    if (m_themes.empty()) return s_empty;
    return m_themes[m_current].name;
}

const SemanticPalette& ThemeManager::semantic() const {
    static const SemanticPalette s_empty;
    if (m_themes.empty()) return s_empty;
    return m_themes[m_current].semantic;
}

void ThemeManager::applyThemeData(const ThemeData& data) {
    applyGeometryData(data.geometry);

    // ImGui colors
    ImGuiStyle& style = ImGui::GetStyle();
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        style.Colors[i] = data.imguiColors[i];
    }

    // ImPlot colors
    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    for (int i = 0; i < ImPlotCol_COUNT; ++i) {
        plotStyle.Colors[i] = data.implotColors[i];
    }

    // ImPlot3D colors
    ImPlot3DStyle& plot3dStyle = ImPlot3D::GetStyle();
    for (int i = 0; i < ImPlot3DCol_COUNT; ++i) {
        plot3dStyle.Colors[i] = data.implot3dColors[i];
    }
}

void ThemeManager::applyGeometryData(const ThemeGeometry& g) {
    ImGuiStyle& style = ImGui::GetStyle();

    // ThemeGeometry stores values in design units (px at 100% DPI).
    // Multiply by the current DPI scale so rounding, padding, spacing
    // and border sizes grow proportionally on HiDPI displays (e.g. 200%).
    // The scale comes from ThemeManager::setDpiScale() (sourced from
    // glfwGetWindowContentScale) because ImGui::GetWindowDpiScale() returns 0
    // before the first frame, when this is called during initialization.
    float dpi = m_dpiScale;
    if (!std::isfinite(dpi) || dpi <= 0.0f) dpi = 1.0f;

    auto scaleV2 = [dpi](const ImVec2& v) {
        return ImVec2(v.x * dpi, v.y * dpi);
    };

    style.WindowRounding    = g.windowRounding    * dpi;
    style.ChildRounding     = g.childRounding     * dpi;
    style.FrameRounding     = g.frameRounding     * dpi;
    style.PopupRounding     = g.popupRounding     * dpi;
    style.ScrollbarRounding = g.scrollbarRounding * dpi;
    style.GrabRounding      = g.grabRounding      * dpi;
    style.TabRounding       = g.tabRounding       * dpi;

    style.WindowPadding     = scaleV2(g.windowPadding);
    style.FramePadding      = scaleV2(g.framePadding);
    style.ItemSpacing       = scaleV2(g.itemSpacing);
    style.ItemInnerSpacing  = scaleV2(g.itemInnerSpacing);
    style.CellPadding       = scaleV2(g.cellPadding);
    style.IndentSpacing     = g.indentSpacing     * dpi;
    style.ScrollbarSize     = g.scrollbarSize     * dpi;
    style.GrabMinSize       = g.grabMinSize       * dpi;

    style.WindowBorderSize  = g.windowBorderSize  * dpi;
    style.ChildBorderSize   = g.childBorderSize   * dpi;
    style.PopupBorderSize   = g.popupBorderSize   * dpi;
    style.FrameBorderSize   = g.frameBorderSize   * dpi;
    style.TabBorderSize     = g.tabBorderSize     * dpi;
}
