#include "gui/theme_manager.h"

// ---------------------------------------------------------------
//  Helper: create an ImVec4 from 0..255 RGBA bytes
// ---------------------------------------------------------------
static ImVec4 col(int r, int g, int b, int a = 255) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

// ===============================================================
//  GitHub-Inspired Light
// ===============================================================
SemanticPalette SemanticPalette::DefaultsFor(bool isLight) {
    SemanticPalette p;
    p.success = ImVec4(0.18f, 0.64f, 0.31f, 1.0f);                                // #2da44e
    p.warning = isLight ? ImVec4(0.75f, 0.55f, 0.05f, 1.0f)                       // #bf8700
                          : ImVec4(0.82f, 0.60f, 0.13f, 1.0f);                    // #d29922
    p.danger  = ImVec4(0.81f, 0.13f, 0.18f, 1.0f);                                // #cf222e
    p.info    = isLight ? ImVec4(0.04f, 0.41f, 0.85f, 1.0f)                       // #0969da
                          : ImVec4(0.35f, 0.65f, 1.00f, 1.0f);                    // #58a6ff
    p.muted   = isLight ? ImVec4(0.40f, 0.45f, 0.50f, 1.0f)                       // #656d76
                          : ImVec4(0.55f, 0.58f, 0.62f, 1.0f);                    // #8b949e

    p.successButton       = ImVec4(0.04f, 0.40f, 0.08f, 1.0f);
    p.successButtonHover  = ImVec4(0.06f, 0.50f, 0.12f, 1.0f);
    p.successButtonActive = ImVec4(0.03f, 0.28f, 0.05f, 1.0f);

    p.dangerButton       = ImVec4(0.55f, 0.10f, 0.08f, 1.0f);
    p.dangerButtonHover  = ImVec4(0.65f, 0.14f, 0.10f, 1.0f);
    p.dangerButtonActive = ImVec4(0.40f, 0.06f, 0.04f, 1.0f);

    p.onAccent = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    return p;
}

ThemeData ThemeData::CreateWin11Light() {
    ThemeData t;
    t.name    = std::string{kThemeNameLight};
    t.isLight = true;
    t.semantic = SemanticPalette::DefaultsFor(true);
    t.clearColor = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    c[ImGuiCol_Text]                 = col(31,  35,  40);   // #1f2328
    c[ImGuiCol_TextDisabled]         = col(101, 109, 118);  // #656d76
    c[ImGuiCol_WindowBg]             = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_ChildBg]              = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_PopupBg]              = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_Border]               = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_FrameBgHovered]       = col(232, 236, 240);  // #e8ecf0
    c[ImGuiCol_FrameBgActive]        = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_TitleBg]              = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_TitleBgActive]        = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_TitleBgCollapsed]     = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_MenuBarBg]            = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_ScrollbarBg]          = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_ScrollbarGrab]        = col(175, 184, 193);  // #afb8c1
    c[ImGuiCol_ScrollbarGrabHovered] = col(143, 154, 165);  // #8f9aa5
    c[ImGuiCol_ScrollbarGrabActive]  = col(118, 130, 142);  // #76828e
    c[ImGuiCol_CheckMark]            = col(9,   105, 218);  // #0969da
    c[ImGuiCol_SliderGrab]           = col(9,   105, 218);  // #0969da
    c[ImGuiCol_SliderGrabActive]     = col(8,   82,  173);  // #0852ad
    c[ImGuiCol_Button]               = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_ButtonHovered]        = col(221, 244, 255);  // #ddf4ff
    c[ImGuiCol_ButtonActive]         = col(9,   105, 218);  // #0969da
    c[ImGuiCol_Header]               = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_HeaderHovered]        = col(221, 244, 255);  // #ddf4ff
    c[ImGuiCol_HeaderActive]         = col(9,   105, 218);  // #0969da
    c[ImGuiCol_Separator]            = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_SeparatorHovered]     = col(9,   105, 218);  // #0969da
    c[ImGuiCol_SeparatorActive]      = col(8,   82,  173);  // #0852ad
    c[ImGuiCol_ResizeGrip]           = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_ResizeGripHovered]    = col(143, 154, 165);  // #8f9aa5
    c[ImGuiCol_ResizeGripActive]     = col(118, 130, 142);  // #76828e
    c[ImGuiCol_InputTextCursor]      = col(9,   105, 218);  // #0969da
    c[ImGuiCol_TabHovered]           = col(232, 236, 240);  // #e8ecf0
    c[ImGuiCol_Tab]                  = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_TabSelected]          = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_TabSelectedOverline]  = col(9,   105, 218);  // #0969da
    c[ImGuiCol_TabDimmed]            = col(232, 236, 240);  // #e8ecf0
    c[ImGuiCol_TabDimmedSelected]    = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_TabDimmedSelectedOverline] = col(101, 109, 118); // #656d76
    c[ImGuiCol_DockingPreview]       = col(9,   105, 218, 90);
    c[ImGuiCol_DockingEmptyBg]       = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_PlotLines]            = col(9,   105, 218);  // #0969da
    c[ImGuiCol_PlotLinesHovered]     = col(8,   82,  173);  // #0852ad
    c[ImGuiCol_PlotHistogram]        = col(9,   105, 218);  // #0969da
    c[ImGuiCol_PlotHistogramHovered] = col(8,   82,  173);  // #0852ad
    c[ImGuiCol_TableHeaderBg]        = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_TableBorderStrong]    = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_TableBorderLight]     = col(232, 236, 240);  // #e8ecf0
    c[ImGuiCol_TableRowBg]           = col(255, 255, 255);  // #ffffff
    c[ImGuiCol_TableRowBgAlt]        = col(246, 248, 250);  // #f6f8fa
    c[ImGuiCol_TextLink]             = col(9,   105, 218);  // #0969da
    c[ImGuiCol_TextSelectedBg]       = col(9,   105, 218, 64);
    c[ImGuiCol_TreeLines]            = col(208, 215, 222);  // #d0d7de
    c[ImGuiCol_DragDropTarget]       = col(9,   105, 218);  // #0969da
    c[ImGuiCol_DragDropTargetBg]     = col(9,   105, 218, 32);
    c[ImGuiCol_UnsavedMarker]        = col(9,   105, 218);  // #0969da
    c[ImGuiCol_NavCursor]            = col(9,   105, 218);  // #0969da
    c[ImGuiCol_NavWindowingHighlight]= col(255, 255, 255);  // #ffffff
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   89);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   115);

    // -- ImPlot -------------------------------------------------
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(246, 248, 250);
    pc[ImPlotCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(31, 35, 40, 51);
    pc[ImPlotCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = col(255, 255, 0);
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

// ===============================================================
//  GitHub Dark-Inspired
// ===============================================================
ThemeData ThemeData::CreateWin11Dark() {
    ThemeData t;
    t.name    = std::string{kThemeNameDark};
    t.isLight = false;
    t.semantic = SemanticPalette::DefaultsFor(false);
    t.clearColor = col(22, 27, 34);  // #161b22

    ImVec4* c = t.imguiColors;

    c[ImGuiCol_Text]                 = col(230, 237, 243);  // #e6edf3
    c[ImGuiCol_TextDisabled]         = col(139, 148, 158);  // #8b949e
    c[ImGuiCol_WindowBg]             = col(22,  27,  34);   // #161b22
    c[ImGuiCol_ChildBg]              = col(13,  17,  23);   // #0d1117
    c[ImGuiCol_PopupBg]              = col(22,  27,  34);   // #161b22
    c[ImGuiCol_Border]               = col(48,  54,  61);   // #30363d
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(33,  38,  45);   // #21262d
    c[ImGuiCol_FrameBgHovered]       = col(48,  54,  61);   // #30363d
    c[ImGuiCol_FrameBgActive]        = col(61,  68,  77);   // #3d444d
    c[ImGuiCol_TitleBg]              = col(13,  17,  23);   // #0d1117
    c[ImGuiCol_TitleBgActive]        = col(22,  27,  34);   // #161b22
    c[ImGuiCol_TitleBgCollapsed]     = col(13,  17,  23);   // #0d1117
    c[ImGuiCol_MenuBarBg]            = col(13,  17,  23);   // #0d1117
    c[ImGuiCol_ScrollbarBg]          = col(22,  27,  34);   // #161b22
    c[ImGuiCol_ScrollbarGrab]        = col(72,  79,  88);   // #484f58
    c[ImGuiCol_ScrollbarGrabHovered] = col(89,  99,  110);  // #59636e
    c[ImGuiCol_ScrollbarGrabActive]  = col(110, 118, 129);  // #6e7681
    c[ImGuiCol_CheckMark]            = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_SliderGrab]           = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_SliderGrabActive]     = col(120, 184, 255);  // #78b8ff
    c[ImGuiCol_Button]               = col(33,  38,  45);   // #21262d
    c[ImGuiCol_ButtonHovered]        = col(31,  42,  63);   // #1f2a3f
    c[ImGuiCol_ButtonActive]         = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_Header]               = col(33,  38,  45);   // #21262d
    c[ImGuiCol_HeaderHovered]        = col(31,  42,  63);   // #1f2a3f
    c[ImGuiCol_HeaderActive]         = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_Separator]            = col(48,  54,  61);   // #30363d
    c[ImGuiCol_SeparatorHovered]     = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_SeparatorActive]      = col(120, 184, 255);  // #78b8ff
    c[ImGuiCol_ResizeGrip]           = col(48,  54,  61);   // #30363d
    c[ImGuiCol_ResizeGripHovered]    = col(89,  99,  110);  // #59636e
    c[ImGuiCol_ResizeGripActive]     = col(110, 118, 129);  // #6e7681
    c[ImGuiCol_InputTextCursor]      = col(230, 237, 243);  // #e6edf3
    c[ImGuiCol_TabHovered]           = col(48,  54,  61);   // #30363d
    c[ImGuiCol_Tab]                  = col(33,  38,  45);   // #21262d
    c[ImGuiCol_TabSelected]          = col(22,  27,  34);   // #161b22
    c[ImGuiCol_TabSelectedOverline]  = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_TabDimmed]            = col(33,  38,  45);   // #21262d
    c[ImGuiCol_TabDimmedSelected]    = col(48,  54,  61);   // #30363d
    c[ImGuiCol_TabDimmedSelectedOverline] = col(139, 148, 158); // #8b949e
    c[ImGuiCol_DockingPreview]       = col(88,  166, 255, 90);
    c[ImGuiCol_DockingEmptyBg]       = col(48,  54,  61);   // #30363d
    c[ImGuiCol_PlotLines]            = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_PlotLinesHovered]     = col(120, 184, 255);  // #78b8ff
    c[ImGuiCol_PlotHistogram]        = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_PlotHistogramHovered] = col(120, 184, 255);  // #78b8ff
    c[ImGuiCol_TableHeaderBg]        = col(33,  38,  45);   // #21262d
    c[ImGuiCol_TableBorderStrong]    = col(48,  54,  61);   // #30363d
    c[ImGuiCol_TableBorderLight]     = col(33,  38,  45);   // #21262d
    c[ImGuiCol_TableRowBg]           = col(22,  27,  34);   // #161b22
    c[ImGuiCol_TableRowBgAlt]        = col(13,  17,  23);   // #0d1117
    c[ImGuiCol_TextLink]             = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_TextSelectedBg]       = col(88,  166, 255, 64);
    c[ImGuiCol_TreeLines]            = col(48,  54,  61);   // #30363d
    c[ImGuiCol_DragDropTarget]       = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_DragDropTargetBg]     = col(88,  166, 255, 32);
    c[ImGuiCol_UnsavedMarker]        = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_NavCursor]            = col(88,  166, 255);  // #58a6ff
    c[ImGuiCol_NavWindowingHighlight]= col(33,  38,  45);   // #21262d
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   115);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   153);

    // -- ImPlot -------------------------------------------------
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(13, 17, 23);
    pc[ImPlotCol_PlotBorder]    = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = t.imguiColors[ImGuiCol_Border];
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(230, 237, 243, 36);
    pc[ImPlotCol_AxisTick]      = pc[ImPlotCol_AxisGrid];
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_ButtonActive];
    pc[ImPlotCol_Selection]     = col(255, 255, 0);
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

// ===============================================================
//  ThemeManager
// ===============================================================
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

void ThemeManager::toggle() {
    // Toggle between light/dark: find first theme with opposite isLight
    if (m_themes.empty()) return;
    bool currentLight = m_themes[m_current].isLight;
    for (size_t i = 0; i < m_themes.size(); ++i) {
        size_t idx = (m_current + 1 + i) % m_themes.size();
        if (m_themes[idx].isLight != currentLight) {
            m_current = idx;
            applyThemeData(m_themes[idx]);
            return;
        }
    }
    // fallback: just go to next
    next();
}

void ThemeManager::next() {
    if (m_themes.empty()) return;
    m_current = (m_current + 1) % m_themes.size();
    applyThemeData(m_themes[m_current]);
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
