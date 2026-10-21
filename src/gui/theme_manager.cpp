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
    t.clearColor = col(248, 248, 248);  // #F8F8F8 (canvas)

    // The single accent color (dark counterpart of the cyan accent).
    constexpr int kAccentR = 27, kAccentG = 94, kAccentB = 158;   // #1b5e9e
    auto accent = [=](int alpha = 255) { return col(kAccentR, kAccentG, kAccentB, alpha); };
    auto accentLight = [=](int alpha = 255) { return col(46, 121, 190, alpha); };  // #2e79be

    // Semantic palette
    t.semantic.success = col(53, 112, 58);        // #35703a
    t.semantic.warning = col(138, 90, 0);         // #8a5a00
    t.semantic.danger  = col(179, 57, 46);        // #b3392e
    t.semantic.info    = accent();                 // #1b5e9e (accent)
    t.semantic.muted   = col(97,  97,  97);        // #616161

    t.semantic.successButton       = col(53, 112, 58);
    t.semantic.successButtonHover  = col(47, 102, 52);
    t.semantic.successButtonActive = col(39, 86, 43);
    t.semantic.dangerButton        = col(179, 57, 46);
    t.semantic.dangerButtonHover   = col(192, 57, 43);
    t.semantic.dangerButtonActive  = col(153, 47, 38);
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    // Surface ladder (see DESIGN.md). Every interactive element shares the
    // "control" color, so the whole theme stays at four neutrals. The light
    // theme follows the VS Code light scheme: a grey chrome with white
    // content, and white fields that carry a hairline border.
    //   canvas #F8F8F8 -> surface #FFFFFF -> control #FFFFFF -> hover #F0F0F0
    c[ImGuiCol_Text]                 = col(38,  40,  42);   // #26282a
    c[ImGuiCol_TextDisabled]         = col(97,  97,  97);  // #616161

    c[ImGuiCol_WindowBg]             = col(248, 248, 248);  // #f8f8f8 canvas
    c[ImGuiCol_ChildBg]              = col(255, 255, 255);  // #ffffff surface
    c[ImGuiCol_PopupBg]              = col(255, 255, 255);  // #ffffff surface
    c[ImGuiCol_MenuBarBg]            = col(248, 248, 248);  // canvas

    c[ImGuiCol_Border]               = col(206, 206, 206);  // #cecece hairline
    c[ImGuiCol_BorderShadow]         = col(0,   0,   0,   15);

    c[ImGuiCol_FrameBg]              = col(255, 255, 255);  // #ffffff control
    c[ImGuiCol_FrameBgHovered]       = col(240, 240, 240);  // #f0f0f0 hover
    c[ImGuiCol_FrameBgActive]        = col(240, 240, 240);  // #f0f0f0 hover

    c[ImGuiCol_TitleBg]              = col(248, 248, 248);  // tab strip = canvas
    c[ImGuiCol_TitleBgActive]        = col(248, 248, 248);
    c[ImGuiCol_TitleBgCollapsed]     = col(248, 248, 248);  // canvas

    c[ImGuiCol_ScrollbarBg]          = col(0,   0,   0,   0);  // transparent: no gutter
    c[ImGuiCol_ScrollbarGrab]        = col(206, 206, 206);  // #cecece
    c[ImGuiCol_ScrollbarGrabHovered] = col(174, 174, 174);  // #aeaeae
    c[ImGuiCol_ScrollbarGrabActive]  = col(174, 174, 174);

    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accentLight();
    c[ImGuiCol_InputTextCursor]      = accent();

    c[ImGuiCol_Button]               = col(255, 255, 255);  // #ffffff control
    c[ImGuiCol_ButtonHovered]        = col(240, 240, 240);  // #f0f0f0 hover
    c[ImGuiCol_ButtonActive]         = col(248, 248, 248);  // canvas: pressed sinks

    c[ImGuiCol_Header]               = col(255, 255, 255);  // #ffffff control
    c[ImGuiCol_HeaderHovered]        = accent(40);   // accent @16%
    c[ImGuiCol_HeaderActive]         = accent(80);   // accent @31%
    c[ImGuiCol_Separator]            = col(229, 229, 229);  // #e5e5e5 separator
    c[ImGuiCol_SeparatorHovered]     = accent();
    c[ImGuiCol_SeparatorActive]      = accent();

    c[ImGuiCol_ResizeGrip]           = accent(40);
    c[ImGuiCol_ResizeGripHovered]    = accent(110);
    c[ImGuiCol_ResizeGripActive]     = accent(170);

    // The active tab carries the accent tint, like the highlighted row of the
    // reference palette: it reads as selected without a bright white slab.
    // The active tab is the color of the chrome, so it continues the panel
    // content instead of standing out as a white slab; the accent overline
    // marks it. The inactive tabs are the darker blocks. This mirrors the
    // dark theme, where the active tab is the lighter one.
    c[ImGuiCol_Tab]                  = col(236, 236, 236);  // #ececec inactive
    c[ImGuiCol_TabHovered]           = col(255, 255, 255);  // #ffffff preview
    c[ImGuiCol_TabSelected]          = col(248, 248, 248);  // #f8f8f8 chrome
    c[ImGuiCol_TabSelectedOverline]  = accent();   // accent underline
    c[ImGuiCol_TabDimmed]            = col(228, 228, 228);  // #e4e4e4 inactive, unfocused
    c[ImGuiCol_TabDimmedSelected]    = col(248, 248, 248);  // #f8f8f8 chrome, unfocused
    c[ImGuiCol_TabDimmedSelectedOverline] = col(97,  97,  97);

    c[ImGuiCol_DockingPreview]       = accent(100);
    c[ImGuiCol_DockingEmptyBg]       = col(248, 248, 248);  // canvas
    c[ImGuiCol_DragDropTarget]       = accent();
    c[ImGuiCol_DragDropTargetBg]     = accent(50);

    c[ImGuiCol_PlotLines]            = accent();
    c[ImGuiCol_PlotLinesHovered]     = accentLight();
    c[ImGuiCol_PlotHistogram]        = accent(180);
    c[ImGuiCol_PlotHistogramHovered] = accentLight(220);
    c[ImGuiCol_TableHeaderBg]        = col(255, 255, 255);  // control
    c[ImGuiCol_TableBorderStrong]    = col(206, 206, 206);  // #cecece hairline
    c[ImGuiCol_TableBorderLight]     = col(229, 229, 229);  // #e5e5e5 separator
    c[ImGuiCol_TableRowBg]           = col(255, 255, 255);  // surface
    c[ImGuiCol_TableRowBgAlt]        = col(240, 240, 240,  255);  // #f0f0f0 hover

    c[ImGuiCol_TextLink]             = accent();
    c[ImGuiCol_TextSelectedBg]       = accent(64);
    c[ImGuiCol_TreeLines]            = col(229, 229, 229);  // #e5e5e5 separator
    c[ImGuiCol_UnsavedMarker]        = accent();
    c[ImGuiCol_NavCursor]            = accent();
    c[ImGuiCol_NavWindowingHighlight]= col(255, 255, 255);  // control
    c[ImGuiCol_NavWindowingDimBg]    = col(0,   0,   0,   100);
    c[ImGuiCol_ModalWindowDimBg]     = col(0,   0,   0,   130);

    // ImPlot
    ImVec4* pc = t.implotColors;
    pc[ImPlotCol_FrameBg]       = t.imguiColors[ImGuiCol_FrameBg];
    pc[ImPlotCol_PlotBg]        = col(248, 248, 248);  // canvas
    pc[ImPlotCol_PlotBorder]    = col(206, 206, 206);  // #cecece hairline
    pc[ImPlotCol_LegendBg]      = t.imguiColors[ImGuiCol_PopupBg];
    pc[ImPlotCol_LegendBorder]  = col(206, 206, 206);  // #cecece hairline
    pc[ImPlotCol_LegendText]    = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_TitleText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_InlayText]     = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisText]      = t.imguiColors[ImGuiCol_Text];
    pc[ImPlotCol_AxisGrid]      = col(36, 39, 44, 28);
    pc[ImPlotCol_AxisTick]      = col(36, 39, 44, 60);
    pc[ImPlotCol_AxisBg]        = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_AxisBgHovered] = t.imguiColors[ImGuiCol_ButtonHovered];
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_Button];
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
    p3[ImPlot3DCol_AxisBgActive]  = t.imguiColors[ImGuiCol_Button];

    // One radius for the whole UI: windows, popups, cards, buttons, frames,
    // tabs, slider grabs and the scrollbar thumb. The thumb is narrow, so the
    // renderer clamps its radius to half its width and it reads as a pill.
    t.geometry.windowRounding    = 4.0f;
    t.geometry.childRounding     = 4.0f;
    t.geometry.popupRounding     = 4.0f;
    t.geometry.frameRounding     = 4.0f;
    t.geometry.tabRounding       = 4.0f;
    t.geometry.scrollbarRounding = 4.0f;
    t.geometry.grabRounding      = 4.0f;
    // The tab border is drawn with ImGuiCol_Border: without it the
    // inactive tabs are plain text on the strip and the active one is
    // a white slab.
    t.geometry.tabBorderSize     = 1.0f;
    t.geometry.frameBorderSize   = 1.0f;

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
    t.semantic.info    = accent();                 // #4296fa (accent)
    t.semantic.muted   = col(148, 154, 171);      // #949aab

    t.semantic.successButton       = col(53, 112, 58);   // #35703a
    t.semantic.successButtonHover  = col(62, 122, 66);   // #3e7a42
    t.semantic.successButtonActive = col(44, 99, 49);    // #2c6331
    t.semantic.dangerButton        = col(179, 57, 46);   // #b3392e
    t.semantic.dangerButtonHover   = col(192, 57, 43);   // #c0392b
    t.semantic.dangerButtonActive  = col(153, 47, 38);   // #992f26
    t.semantic.onAccent            = col(255, 255, 255);

    ImVec4* c = t.imguiColors;

    // Surface ladder (see DESIGN.md). Every interactive element shares the
    // "control" color, so the whole theme stays at four neutrals:
    //   canvas #0F0F0F -> surface #161719 -> control #1D1F23 -> hover #262A31
    c[ImGuiCol_Text]                 = col(201, 204, 214); // #c9ccd6
    c[ImGuiCol_TextDisabled]         = col(148, 154, 171); // #949aab
    c[ImGuiCol_WindowBg]             = col(15,  15,  15);   // #0f0f0f canvas
    c[ImGuiCol_ChildBg]              = col(22,  23,  25);   // #161719 surface
    c[ImGuiCol_PopupBg]              = col(22,  23,  25);   // #161719 surface
    c[ImGuiCol_MenuBarBg]            = col(15,  15,  15);   // canvas
    c[ImGuiCol_Border]               = col(38,  42,  49);   // #262a31 hairline
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = col(29,  31,  35);   // #1d1f23 control
    c[ImGuiCol_FrameBgHovered]       = col(38,  42,  49);   // #262a31 hover
    c[ImGuiCol_FrameBgActive]        = col(38,  42,  49);   // hover
    c[ImGuiCol_TitleBg]              = col(15,  15,  15);   // tab strip = canvas
    c[ImGuiCol_TitleBgActive]        = col(15,  15,  15);
    c[ImGuiCol_TitleBgCollapsed]     = col(15,  15,  15);   // canvas
    c[ImGuiCol_MenuBarBg]            = col(15,  15,  15);
    // The scrollbar track stays transparent: ImGui paints it with the
    // window's corner radius, so a filled gutter inside a card ends in a
    // second arc next to the card's own rounded corner. Only the thumb is
    // drawn, and it uses the same radius as the card.
    c[ImGuiCol_ScrollbarBg]          = col(0,   0,   0,   0);  // transparent: no gutter
    c[ImGuiCol_ScrollbarGrab]        = col(38,  42,  49);   // #262a31
    c[ImGuiCol_ScrollbarGrabHovered] = col(54,  60,  70);   // #363c46 hover
    c[ImGuiCol_ScrollbarGrabActive]  = col(54,  60,  70);   // hover
    // Accent: the single blue accent (distinct from success green).
    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accentLight();
    c[ImGuiCol_InputTextCursor]      = accent();
    c[ImGuiCol_Button]               = col(29,  31,  35);   // #1d1f23 control
    c[ImGuiCol_ButtonHovered]        = col(38,  42,  49);   // #262a31 hover
    c[ImGuiCol_ButtonActive]         = col(22,  23,  25);   // #161719 pressed sinks
    c[ImGuiCol_Header]               = col(29,  31,  35);   // #1d1f23 control
    c[ImGuiCol_HeaderHovered]        = accent(40);   // accent @16%
    c[ImGuiCol_HeaderActive]         = accent(80);   // accent @31%
    c[ImGuiCol_Separator]            = col(38,  42,  49);   // #262a31 hairline
    c[ImGuiCol_SeparatorHovered]     = accent();
    c[ImGuiCol_SeparatorActive]      = accent();
    c[ImGuiCol_ResizeGrip]           = accent(40);
    c[ImGuiCol_ResizeGripHovered]    = accent(110);
    c[ImGuiCol_ResizeGripActive]     = accent(170);
    c[ImGuiCol_TabHovered]           = col(29,  31,  35);   // control
    c[ImGuiCol_Tab]                  = col(22,  23,  25);   // #161719 surface
    c[ImGuiCol_TabSelected]          = col(29,  31,  35);   // #1d1f23 control
    c[ImGuiCol_TabSelectedOverline]  = accent();       // accent underline
    c[ImGuiCol_TabDimmed]            = col(22,  23,  25);   // surface
    c[ImGuiCol_TabDimmedSelected]    = col(29,  31,  35);   // control
    c[ImGuiCol_TabDimmedSelectedOverline] = col(148, 154, 171);
    c[ImGuiCol_DockingPreview]       = accent(90);
    c[ImGuiCol_DockingEmptyBg]       = col(22,  23,  25);   // surface
    c[ImGuiCol_PlotLines]            = col(155, 160, 172); // #9ba0ac
    c[ImGuiCol_PlotLinesHovered]     = accentLight();
    c[ImGuiCol_PlotHistogram]        = accent(180);
    c[ImGuiCol_PlotHistogramHovered] = accentLight(220);
    c[ImGuiCol_TableHeaderBg]        = col(29,  31,  35);   // control
    c[ImGuiCol_TableBorderStrong]    = col(38,  42,  49);   // #262a31 hairline
    c[ImGuiCol_TableBorderLight]     = col(38,  42,  49);   // hairline
    c[ImGuiCol_TableRowBg]           = col(22,  23,  25);   // surface
    c[ImGuiCol_TableRowBgAlt]        = col(29,  31,  35,  64);  // control @25%
    c[ImGuiCol_TextLink]             = accent();
    c[ImGuiCol_TextSelectedBg]       = accent(80);
    c[ImGuiCol_TreeLines]            = col(38,  42,  49);   // #262a31 hairline
    c[ImGuiCol_DragDropTarget]       = accent();
    c[ImGuiCol_DragDropTargetBg]     = accent(32);
    c[ImGuiCol_UnsavedMarker]        = accent();
    c[ImGuiCol_NavCursor]            = accent();
    c[ImGuiCol_NavWindowingHighlight]= col(29,  31,  35);   // control
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
    pc[ImPlotCol_AxisBgActive]  = t.imguiColors[ImGuiCol_Button];
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
    p3[ImPlot3DCol_AxisBgActive]  = t.imguiColors[ImGuiCol_Button];

    // One radius for the whole UI: windows, popups, cards, buttons, frames,
    // tabs, slider grabs and the scrollbar thumb. The thumb is narrow, so the
    // renderer clamps its radius to half its width and it reads as a pill.
    t.geometry.windowRounding    = 4.0f;
    t.geometry.childRounding     = 4.0f;
    t.geometry.popupRounding     = 4.0f;
    t.geometry.frameRounding     = 4.0f;
    t.geometry.tabRounding       = 4.0f;
    t.geometry.scrollbarRounding = 4.0f;
    t.geometry.grabRounding      = 4.0f;
    t.geometry.frameBorderSize   = 0.0f;

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
