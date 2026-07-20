#include "gui/theme_manager.h"

// ---------------------------------------------------------------
//  Helper: create an ImVec4 from 0..255 RGBA bytes
// ---------------------------------------------------------------
static ImVec4 col(int r, int g, int b, int a = 255) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

SemanticPalette SemanticPalette::DefaultsFor(bool isLight) {
    SemanticPalette p;

    if (isLight) {
        p.success = ImVec4(0.063f, 0.486f, 0.063f, 1.0f);  // #107c10
        p.warning = ImVec4(0.792f, 0.314f, 0.063f, 1.0f);  // #ca5010
        p.danger  = ImVec4(0.820f, 0.204f, 0.220f, 1.0f);  // #d13438
        p.info    = ImVec4(0.000f, 0.471f, 0.831f, 1.0f);  // #0078d4
        p.muted   = ImVec4(0.420f, 0.420f, 0.420f, 1.0f);  // #6b6b6b

        p.successButton       = ImVec4(0.063f, 0.486f, 0.063f, 1.0f);
        p.successButtonHover  = ImVec4(0.094f, 0.612f, 0.094f, 1.0f);
        p.successButtonActive = ImVec4(0.039f, 0.353f, 0.039f, 1.0f);

        p.dangerButton       = ImVec4(0.820f, 0.204f, 0.220f, 1.0f);
        p.dangerButtonHover  = ImVec4(0.890f, 0.298f, 0.314f, 1.0f);
        p.dangerButtonActive = ImVec4(0.671f, 0.149f, 0.165f, 1.0f);

        p.neutralButton       = ImVec4(0.839f, 0.839f, 0.839f, 1.0f);  // #d6d6d6
        p.neutralButtonHover  = ImVec4(0.890f, 0.890f, 0.890f, 1.0f);  // #e3e3e3
        p.neutralButtonActive = ImVec4(0.780f, 0.780f, 0.780f, 1.0f);  // #c7c7c7

        p.onAccent = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        p.success = ImVec4(0.247f, 0.725f, 0.314f, 1.0f);  // #3fb950  (GitHub Dark Primer green)
        p.warning = ImVec4(0.988f, 0.882f, 0.000f, 1.0f);  // #fce100
        p.danger  = ImVec4(0.973f, 0.318f, 0.286f, 1.0f);  // #f85149  (GitHub Dark Primer red)
        p.info    = ImVec4(0.259f, 0.588f, 0.976f, 1.0f);  // #4296f9
        p.muted   = ImVec4(0.541f, 0.541f, 0.541f, 1.0f);  // #8a8a8a

        p.successButton       = ImVec4(0.137f, 0.525f, 0.212f, 1.0f);  // #238636
        p.successButtonHover  = ImVec4(0.180f, 0.627f, 0.263f, 1.0f);  // #2ea043
        p.successButtonActive = ImVec4(0.102f, 0.498f, 0.216f, 1.0f);  // #1a7f37

        p.dangerButton       = ImVec4(0.675f, 0.180f, 0.157f, 1.0f);  // #ac2e28  (deep saturated red)
        p.dangerButtonHover  = ImVec4(0.792f, 0.216f, 0.184f, 1.0f);  // #ca372f  (hover)
        p.dangerButtonActive = ImVec4(0.518f, 0.141f, 0.122f, 1.0f);  // #84241f  (active)

        p.neutralButton       = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);  // #2d2d2d
        p.neutralButtonHover  = ImVec4(0.220f, 0.220f, 0.220f, 1.0f);  // #383838
        p.neutralButtonActive = ImVec4(0.271f, 0.271f, 0.271f, 1.0f);  // #454545

        p.onAccent = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return p;
}

ThemeData ThemeData::CreateThemeLight() {
    ThemeData t;
    t.name    = std::string{kThemeNameLight};
    t.isLight = true;
    t.semantic = SemanticPalette::DefaultsFor(true);
    t.clearColor = col(243, 243, 243);  // #f3f3f3

    ImVec4* c = t.imguiColors;

    c[ImGuiCol_Text]                 = col(31,  31,  31);   // #1f1f1f
    c[ImGuiCol_TextDisabled]         = col(107, 107, 107);  // #6b6b6b
    c[ImGuiCol_WindowBg]             = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_ChildBg]              = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_PopupBg]              = col(252, 252, 252);  // #fcfcfc
    c[ImGuiCol_Border]               = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_FrameBgHovered]       = col(240, 240, 240);  // #f0f0f0
    c[ImGuiCol_FrameBgActive]        = col(224, 224, 224);  // #e0e0e0
    c[ImGuiCol_TitleBg]              = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_TitleBgActive]        = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_TitleBgCollapsed]     = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_MenuBarBg]            = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_ScrollbarBg]          = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_ScrollbarGrab]        = col(200, 200, 200);  // #c8c8c8
    c[ImGuiCol_ScrollbarGrabHovered] = col(170, 170, 170);  // #aaaaaa
    c[ImGuiCol_ScrollbarGrabActive]  = col(140, 140, 140);  // #8c8c8c
    c[ImGuiCol_CheckMark]            = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_SliderGrab]           = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_SliderGrabActive]     = col(0,   90,  158);  // #005a9e
    c[ImGuiCol_Button]               = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_ButtonHovered]        = col(240, 240, 240);  // #f0f0f0
    c[ImGuiCol_ButtonActive]         = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_Header]               = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_HeaderHovered]        = col(0,   120, 212, 26); // rgba(0,120,212,0.10)
    c[ImGuiCol_HeaderActive]         = col(0,   120, 212, 51); // rgba(0,120,212,0.20)
    c[ImGuiCol_Separator]            = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_SeparatorHovered]     = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_SeparatorActive]      = col(0,   90,  158);  // #005a9e
    c[ImGuiCol_ResizeGrip]           = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_ResizeGripHovered]    = col(170, 170, 170);  // #aaaaaa
    c[ImGuiCol_ResizeGripActive]     = col(140, 140, 140);  // #8c8c8c
    c[ImGuiCol_InputTextCursor]      = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_TabHovered]           = col(240, 240, 240);  // #f0f0f0
    c[ImGuiCol_Tab]                  = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_TabSelected]          = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_TabSelectedOverline]  = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_TabDimmed]            = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_TabDimmedSelected]    = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_TabDimmedSelectedOverline] = col(107, 107, 107); // #6b6b6b
    c[ImGuiCol_DockingPreview]       = col(0,   120, 212, 90);
    c[ImGuiCol_DockingEmptyBg]       = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_PlotLines]            = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_PlotLinesHovered]     = col(0,   90,  158);  // #005a9e
    c[ImGuiCol_PlotHistogram]        = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_PlotHistogramHovered] = col(0,   90,  158);  // #005a9e
    c[ImGuiCol_TableHeaderBg]        = col(243, 243, 243);  // #f3f3f3
    c[ImGuiCol_TableBorderStrong]    = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_TableBorderLight]     = col(235, 235, 235);  // #ebebeb
    c[ImGuiCol_TableRowBg]           = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_TableRowBgAlt]        = col(250, 250, 250);  // #fafafa
    c[ImGuiCol_TextLink]             = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_TextSelectedBg]       = col(0,   120, 212, 64);
    c[ImGuiCol_TreeLines]            = col(229, 229, 229);  // #e5e5e5
    c[ImGuiCol_DragDropTarget]       = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_DragDropTargetBg]     = col(0,   120, 212, 32);
    c[ImGuiCol_UnsavedMarker]        = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_NavCursor]            = col(0,   120, 212);  // #0078d4
    c[ImGuiCol_NavWindowingHighlight]= col(255, 255, 255);  // #ffffff
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   89);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   115);

    // -- ImPlot -------------------------------------------------
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(252, 252, 252);
    pc[ImPlotCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(31, 31, 31, 38);
    pc[ImPlotCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = col(0, 120, 212, 64);
    pc[ImPlotCol_Crosshairs]    = pc[ImPlotCol_PlotBorder];

    // -- ImPlot3D -----------------------------------------------
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

ThemeData ThemeData::CreateThemeDark() {
    ThemeData t;
    t.name    = std::string{kThemeNameDark};
    t.isLight = false;
    t.semantic = SemanticPalette::DefaultsFor(false);
    t.clearColor = col(24, 24, 24);  // #181818

    t.geometry.frameRounding = 8.0f;
    t.geometry.tabRounding   = 8.0f;

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
    c[ImGuiCol_Header]               = col(50,  104, 173);      // #3268ad
    c[ImGuiCol_HeaderHovered]        = col(58,  125, 204);      // #3a7dcc
    c[ImGuiCol_HeaderActive]         = col(27,  75,  126);      // #1b4b7e
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
    c[ImGuiCol_PlotHistogram]        = col(229, 178, 0);    // #e5b200
    c[ImGuiCol_PlotHistogramHovered] = col(255, 153, 0);    // #ff9900
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

    // -- ImPlot -------------------------------------------------
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

    // -- ImPlot3D -----------------------------------------------
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

    // -- ImGui colors --
    ImGuiStyle& style = ImGui::GetStyle();
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        style.Colors[i] = data.imguiColors[i];
    }

    // -- ImPlot colors --
    ImPlotStyle& plotStyle = ImPlot::GetStyle();
    for (int i = 0; i < ImPlotCol_COUNT; ++i) {
        plotStyle.Colors[i] = data.implotColors[i];
    }

    // -- ImPlot3D colors --
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
