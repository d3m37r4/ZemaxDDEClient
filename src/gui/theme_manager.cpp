#include "gui/theme_manager.h"

// Helper: create an ImVec4 from 0..255 RGBA bytes
static ImVec4 col(int r, int g, int b, int a = 255) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

// Light Theme
ThemeData ThemeData::CreateThemeLight() {
    ThemeData t;
    t.name    = std::string{kThemeNameLight};
    t.isLight = true;
    t.clearColor = col(255, 255, 255);  // #FFFFFF

    // Semantic palette
    t.semantic.success = col(22, 130, 93);       // #16825d
    t.semantic.warning = col(154, 103, 0);       // #9a6700
    t.semantic.danger  = col(196, 43, 28);       // #c42b1c
    t.semantic.info    = col(0, 102, 191);       // #0066bf
    t.semantic.muted   = col(97, 97, 97);        // #616161

    t.semantic.successButton       = col(22, 130, 93);
    t.semantic.successButtonHover  = col(26, 148, 103);
    t.semantic.successButtonActive = col(14, 107, 71);
    t.semantic.dangerButton        = col(196, 43, 28);
    t.semantic.dangerButtonHover   = col(212, 53, 35);
    t.semantic.dangerButtonActive  = col(165, 32, 22);
    t.semantic.neutralButton       = col(243, 244, 246); // #F3F4F6
    t.semantic.neutralButtonHover  = col(229, 231, 235); // #E5E7EB
    t.semantic.neutralButtonActive = col(209, 213, 219); // #D1D5DB
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    c[ImGuiCol_Text]                 = col(31,  31,  31);   // #1F1F1F
    c[ImGuiCol_TextDisabled]         = col(107, 107, 107);  // #6B6B6B

    c[ImGuiCol_WindowBg]             = col(255, 255, 255);  // #FFFFFF
    c[ImGuiCol_ChildBg]              = col(248, 249, 250);  // #F8F9FA
    c[ImGuiCol_PopupBg]              = col(255, 255, 255, 250);
    c[ImGuiCol_MenuBarBg]            = col(255, 255, 255);

    c[ImGuiCol_Border]               = col(229, 231, 235);  // #E5E7EB
    c[ImGuiCol_BorderShadow]         = col(0,   0,   0,   15);

    c[ImGuiCol_FrameBg]              = col(255, 255, 255);
    c[ImGuiCol_FrameBgHovered]       = col(249, 250, 251);
    c[ImGuiCol_FrameBgActive]        = col(255, 255, 255);

    c[ImGuiCol_TitleBg]              = col(248, 249, 250);
    c[ImGuiCol_TitleBgActive]        = col(255, 255, 255);
    c[ImGuiCol_TitleBgCollapsed]     = col(243, 244, 246);

    c[ImGuiCol_ScrollbarBg]          = col(255, 255, 255);
    c[ImGuiCol_ScrollbarGrab]        = col(209, 213, 219);
    c[ImGuiCol_ScrollbarGrabHovered] = col(156, 163, 175);
    c[ImGuiCol_ScrollbarGrabActive]  = col(107, 114, 128);

    c[ImGuiCol_CheckMark]            = col(0,   120, 212);  // #0078D4
    c[ImGuiCol_SliderGrab]           = col(0,   120, 212);
    c[ImGuiCol_SliderGrabActive]     = col(0,   90,  158);
    c[ImGuiCol_InputTextCursor]      = col(0,   120, 212);

    c[ImGuiCol_Button]               = col(243, 244, 246);  // #F3F4F6
    c[ImGuiCol_ButtonHovered]        = col(229, 231, 235);  // #E5E7EB
    c[ImGuiCol_ButtonActive]         = col(0,   120, 212);  // #0078D4

    c[ImGuiCol_Header]               = col(243, 244, 246);
    c[ImGuiCol_HeaderHovered]        = col(0,   120, 212, 26);
    c[ImGuiCol_HeaderActive]         = col(0,   120, 212, 51);
    c[ImGuiCol_Separator]            = col(229, 231, 235);
    c[ImGuiCol_SeparatorHovered]     = col(0,   120, 212);
    c[ImGuiCol_SeparatorActive]      = col(0,   90,  158);

    c[ImGuiCol_ResizeGrip]           = col(209, 213, 219);
    c[ImGuiCol_ResizeGripHovered]    = col(156, 163, 175);
    c[ImGuiCol_ResizeGripActive]     = col(107, 114, 128);

    c[ImGuiCol_Tab]                  = col(243, 244, 246);
    c[ImGuiCol_TabHovered]           = col(229, 231, 235);
    c[ImGuiCol_TabSelected]          = col(255, 255, 255);
    c[ImGuiCol_TabSelectedOverline]  = col(0,   120, 212);
    c[ImGuiCol_TabDimmed]            = col(243, 244, 246);
    c[ImGuiCol_TabDimmedSelected]    = col(255, 255, 255);
    c[ImGuiCol_TabDimmedSelectedOverline] = col(107, 114, 128);

    c[ImGuiCol_DockingPreview]       = col(0,   120, 212, 100);
    c[ImGuiCol_DockingEmptyBg]       = col(243, 244, 246);
    c[ImGuiCol_DragDropTarget]       = col(0,   120, 212);
    c[ImGuiCol_DragDropTargetBg]     = col(0,   120, 212, 50);

    c[ImGuiCol_PlotLines]            = col(0,   120, 212);
    c[ImGuiCol_PlotLinesHovered]     = col(0,   90,  158);
    c[ImGuiCol_PlotHistogram]        = col(0,   120, 212, 180);
    c[ImGuiCol_PlotHistogramHovered] = col(0,   90,  158, 220);
    c[ImGuiCol_TableHeaderBg]        = col(248, 249, 250);
    c[ImGuiCol_TableBorderStrong]    = col(229, 231, 235);
    c[ImGuiCol_TableBorderLight]     = col(243, 244, 246);
    c[ImGuiCol_TableRowBg]           = col(255, 255, 255);
    c[ImGuiCol_TableRowBgAlt]        = col(249, 250, 251);

    c[ImGuiCol_TextLink]             = col(0,   120, 212);
    c[ImGuiCol_TextSelectedBg]       = col(0,   120, 212, 64);
    c[ImGuiCol_TreeLines]            = col(229, 231, 235);
    c[ImGuiCol_UnsavedMarker]        = col(239, 68,  68);   // #EF4444
    c[ImGuiCol_NavCursor]            = col(0,   120, 212);
    c[ImGuiCol_NavWindowingHighlight]= col(255, 255, 255);
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   100);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   130);

    // ImPlot
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(255, 255, 255);
    pc[ImPlotCol_PlotBorder]    = col(229, 231, 235);
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = col(229, 231, 235);
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(229, 231, 235);       // #E5E7EB
    pc[ImPlotCol_AxisTick]      = col(156, 163, 175);
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = col(0, 120, 212, 60);
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

    return t;
}

// Dark Theme
ThemeData ThemeData::CreateThemeDark() {
    ThemeData t;
    t.name    = std::string{kThemeNameDark};
    t.isLight = false;
    t.clearColor = col(24, 24, 24);  // #181818

    // Semantic palette
    t.semantic.success = col(63, 185, 80);        // #3fb950
    t.semantic.warning = col(252, 225, 0);        // #fce100
    t.semantic.danger  = col(248, 81, 73);        // #f85149
    t.semantic.info    = col(66, 150, 249);       // #4296f9
    t.semantic.muted   = col(138, 138, 138);      // #8a8a8a

    t.semantic.successButton       = col(35, 134, 54);   // #238636
    t.semantic.successButtonHover  = col(46, 160, 67);   // #2ea043
    t.semantic.successButtonActive = col(26, 127, 55);   // #1a7f37
    t.semantic.dangerButton        = col(172, 46, 40);   // #ac2e28
    t.semantic.dangerButtonHover   = col(202, 55, 47);   // #ca372f
    t.semantic.dangerButtonActive  = col(132, 36, 31);   // #84241f
    t.semantic.neutralButton       = col(45, 45, 45);    // #2d2d2d
    t.semantic.neutralButtonHover  = col(56, 56, 56);    // #383838
    t.semantic.neutralButtonActive = col(69, 69, 69);    // #454545
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    c[ImGuiCol_Text]                 = col(240, 240, 240);  // #f0f0f0
    c[ImGuiCol_TextDisabled]         = col(138, 138, 138);  // #8a8a8a
    c[ImGuiCol_WindowBg]             = col(24,  24,  24);   // #181818
    c[ImGuiCol_ChildBg]              = col(24,  24,  24);   // #181818
    c[ImGuiCol_PopupBg]              = col(37,  37,  37);   // #252525
    c[ImGuiCol_Border]               = col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_FrameBgHovered]       = col(56,  56,  56);   // #383838
    c[ImGuiCol_FrameBgActive]        = col(69,  69,  69);   // #454545
    c[ImGuiCol_TitleBg]              = col(24,  24,  24);   // #181818
    c[ImGuiCol_TitleBgActive]        = col(24,  24,  24);   // #181818
    c[ImGuiCol_TitleBgCollapsed]     = col(18,  18,  18);   // #121212
    c[ImGuiCol_MenuBarBg]            = col(24,  24,  24);   // #181818
    c[ImGuiCol_ScrollbarBg]          = col(24,  24,  24);   // #181818
    c[ImGuiCol_ScrollbarGrab]        = col(80,  80,  80);   // #505050
    c[ImGuiCol_ScrollbarGrabHovered] = col(110, 110, 110);  // #6e6e6e
    c[ImGuiCol_ScrollbarGrabActive]  = col(140, 140, 140);  // #8c8c8c
    c[ImGuiCol_CheckMark]            = col(66,  150, 250);  // #4296fa
    c[ImGuiCol_SliderGrab]           = col(61,  133, 224);  // #3d85e0
    c[ImGuiCol_SliderGrabActive]     = col(66,  150, 250);  // #4296fa
    c[ImGuiCol_Button]               = col(50,  104, 173);  // #3268ad
    c[ImGuiCol_ButtonHovered]        = col(58,  125, 204);  // #3a7dcc
    c[ImGuiCol_ButtonActive]         = col(27,  75,  126);  // #1b4b7e
    c[ImGuiCol_Header]               = col(50,  104, 173);  // #3268ad
    c[ImGuiCol_HeaderHovered]        = col(58,  125, 204);  // #3a7dcc
    c[ImGuiCol_HeaderActive]         = col(27,  75,  126);  // #1b4b7e
    c[ImGuiCol_Separator]            = col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_SeparatorHovered]     = col(25,  102, 191, 199); // #1966bf @78%
    c[ImGuiCol_SeparatorActive]      = col(25,  102, 191);  // #1966bf
    c[ImGuiCol_ResizeGrip]           = col(66,  150, 249, 51);  // #4296f9 @20%
    c[ImGuiCol_ResizeGripHovered]    = col(66,  150, 249, 171); // #4296f9 @67%
    c[ImGuiCol_ResizeGripActive]     = col(66,  150, 249, 242); // #4296f9 @95%
    c[ImGuiCol_InputTextCursor]      = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_TabHovered]           = col(66,  150, 249, 204); // #4296f9 @80%
    c[ImGuiCol_Tab]                  = col(32,  32,  32);   // #202020
    c[ImGuiCol_TabSelected]          = col(50,  104, 173);  // #3268ad
    c[ImGuiCol_TabSelectedOverline]  = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_TabDimmed]            = col(32,  32,  32);   // #202020
    c[ImGuiCol_TabDimmedSelected]    = col(32,  32,  32);   // #202020
    c[ImGuiCol_TabDimmedSelectedOverline] = col(138, 138, 138); // #8a8a8a
    c[ImGuiCol_DockingPreview]       = col(66,  150, 249, 90);
    c[ImGuiCol_DockingEmptyBg]       = col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_PlotLines]            = col(155, 155, 155);  // #9b9b9b
    c[ImGuiCol_PlotLinesHovered]     = col(255, 109, 89);   // #ff6d59
    c[ImGuiCol_PlotHistogram]        = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_PlotHistogramHovered] = col(100, 170, 255);  // lighter blue
    c[ImGuiCol_TableHeaderBg]        = col(48,  48,  51);   // #303033
    c[ImGuiCol_TableBorderStrong]    = col(79,  79,  89);   // #4f4f59
    c[ImGuiCol_TableBorderLight]     = col(58,  58,  63);   // #3a3a3f
    c[ImGuiCol_TableRowBg]           = col(32,  32,  32);   // #202020
    c[ImGuiCol_TableRowBgAlt]        = col(255, 255, 255, 15); // near-transparent white
    c[ImGuiCol_TextLink]             = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_TextSelectedBg]       = col(66,  150, 249, 89);  // #4296f9 @35%
    c[ImGuiCol_TreeLines]            = col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_DragDropTarget]       = col(255, 255, 0);    // #ffff00
    c[ImGuiCol_DragDropTargetBg]     = col(255, 255, 0, 32); // #ffff00 @13%
    c[ImGuiCol_UnsavedMarker]        = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_NavCursor]            = col(66,  150, 249);  // #4296f9
    c[ImGuiCol_NavWindowingHighlight]= col(45,  45,  45);   // #2d2d2d
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   115);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   153);

    // ImPlot
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(24, 24, 24);
    pc[ImPlotCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(240, 240, 240, 36);
    pc[ImPlotCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = col(66, 150, 249, 64);
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

    style.WindowRounding    = g.windowRounding;
    style.ChildRounding     = g.childRounding;
    style.FrameRounding     = g.frameRounding;
    style.PopupRounding     = g.popupRounding;
    style.ScrollbarRounding = g.scrollbarRounding;
    style.GrabRounding      = g.grabRounding;
    style.TabRounding       = g.tabRounding;

    style.WindowPadding     = g.windowPadding;
    style.FramePadding      = g.framePadding;
    style.ItemSpacing       = g.itemSpacing;
    style.ItemInnerSpacing  = g.itemInnerSpacing;
    style.CellPadding       = g.cellPadding;
    style.IndentSpacing     = g.indentSpacing;
    style.ScrollbarSize     = g.scrollbarSize;
    style.GrabMinSize       = g.grabMinSize;
    style.WindowBorderSize  = g.windowBorderSize;
    style.ChildBorderSize   = g.childBorderSize;
    style.PopupBorderSize   = g.popupBorderSize;
    style.FrameBorderSize   = g.frameBorderSize;
    style.TabBorderSize     = g.tabBorderSize;
}
