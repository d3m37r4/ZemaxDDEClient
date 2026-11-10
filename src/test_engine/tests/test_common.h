#pragma once

// Shared helpers for the application test suites.
//
// The tests drive the real UI: they click menu entries, toggle dock panels
// and inspect the resulting application state. Everything they assert on is
// therefore reachable through the public API of the GUI layer.
//
// NOTE: the application headers must come first: they pull in <windows.h>,
// which defines a `Yield()` macro that would break ImGuiTestContext::Yield().
// The engine headers undo that macro, but only when they are included last.

#include "gui/constants.h"
#include "gui/dockable_windows_manager.h"
#include "gui/gui.h"
#include "gui/settings_manager.h"
#include "gui/theme_manager.h"

#include "test_engine/test_engine.h"

#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_engine.h"

namespace app::testing {

/// Retrieve the shared context from a running test.
inline TestAppContext* appContext(ImGuiTestContext* ctx) {
    return static_cast<TestAppContext*>(ctx->Test->UserData);
}

/// The main menu bar lives in the "##MainMenuBar" window.
inline constexpr const char* kMenuBar = "##MainMenuBar";

/// Path of an entry of the main menu bar, e.g. "//##MainMenuBar/View/Logs".
///
/// The menu and item labels contain a FontAwesome icon in front of the visible
/// text (e.g. "\xEF\x81\xAE View"), and the test engine matches items by
/// hashing that exact label. The icon macros are therefore reused here so the
/// bytes are identical to the ones the application submits.
inline std::string menuPath(const char* menuLabelWithIcon, const char* itemLabel = nullptr) {
    std::string path = "//";
    path += kMenuBar;
    path += '/';
    path += menuLabelWithIcon;
    if (itemLabel) {
        path += '/';
        path += itemLabel;
    }
    return path;
}

/// Click a main menu bar entry, e.g. menuPath(ICON_FA_EYE " View", "Logs").
///
/// MenuClick() is used rather than ItemClick() because the engine has to open
/// the parent menu before the item becomes reachable.
inline void clickMenuItem(ImGuiTestContext* ctx, const char* menuLabelWithIcon, const char* itemLabel) {
    ctx->MenuClick(menuPath(menuLabelWithIcon, itemLabel).c_str());
    ctx->Yield();
}

/// Click a main menu bar entry and check it toggled the visibility of a panel.
inline void togglePanel(ImGuiTestContext* ctx, DockableWindowsManager* windows,
                         WindowID id, const char* panelLabel) {
    const bool wasVisible = windows->IsVisible(id);

    clickMenuItem(ctx, ICON_FA_EYE " View", panelLabel);
    IM_CHECK_NE(windows->IsVisible(id), wasVisible);

    clickMenuItem(ctx, ICON_FA_EYE " View", panelLabel);
    IM_CHECK_EQ(windows->IsVisible(id), wasVisible);
}

/// Set the reference to a child window of a popup, so that its items can be
/// addressed with a relative path.
///
/// The Preferences dialog splits its content into the "##prefs_sidebar" and
/// "##prefs_content" child windows; items inside them are only reachable
/// relative to that child window.
inline void setRefToChildWindow(ImGuiTestContext* ctx, const char* popupName, const char* childName) {
    ImGuiTestItemInfo child = ctx->WindowInfo((std::string{"//"} + popupName + "/" + childName).c_str(),
                                              ImGuiTestOpFlags_NoError);
    IM_CHECK(child.Window != nullptr);
    if (child.Window) {
        ctx->SetRef(child.Window);
    }
}

/// Click a button of a modal dialog, e.g. "//Preferences/Save".
inline void clickDialogButton(ImGuiTestContext* ctx, const char* popupName, const char* label) {
    ctx->ItemClick((std::string{"//"} + popupName + "/" + label).c_str());
    ctx->Yield();
}

/// The theme that is currently applied to the UI.
///
/// Note: this differs from the persisted theme mode. Picking a radio button
/// applies the theme immediately (live preview) but only "Save" writes it to
/// the settings.
inline std::string appliedTheme(TestAppContext* app) {
    auto* themeManager = app->gui->getSettingsManager().getThemeManager();
    return themeManager ? std::string{themeManager->currentThemeName()} : std::string{};
}

/// The radio button label of a theme mode.
inline const char* themeRadioLabel(app::ThemeMode mode) {
    switch (mode) {
        case app::ThemeMode::Light: return ICON_FA_SUN " Light";
        case app::ThemeMode::Dark:  return ICON_FA_MOON " Dark";
        default:                     return ICON_FA_DISPLAY " System";
    }
}

/// The theme name a mode resolves to.
inline std::string themeNameOf(app::ThemeMode mode) {
    switch (mode) {
        case app::ThemeMode::Light: return std::string{kThemeNameLight};
        case app::ThemeMode::Dark:  return std::string{kThemeNameDark};
        default:                     return std::string{}; // System depends on the OS setting.
    }
}

/// Open the Preferences dialog and select its Appearance section.
///
/// The dialog is only opened when it is not open already: clicking the menu
/// entry a second time would toggle the (already open) menu shut.
inline void openAppearanceSection(ImGuiTestContext* ctx, TestAppContext* app) {
    if (!app->gui->isAnyModalOpen()) {
        clickMenuItem(ctx, ICON_FA_FOLDER_OPEN " File", ICON_FA_GEAR " Preferences");
    }
    setRefToChildWindow(ctx, "Preferences", "##prefs_sidebar");
    ctx->ItemClick(ICON_FA_PALETTE " Appearance");
    ctx->Yield();
}

/// Pick a theme radio button of the Appearance section.
inline void clickThemeRadio(ImGuiTestContext* ctx, app::ThemeMode mode) {
    setRefToChildWindow(ctx, "Preferences", "##prefs_content");
    ctx->ItemClick(themeRadioLabel(mode));
    ctx->Yield();
}

/// Make sure no modal dialog is left open and all panels are back to the
/// default visibility (DDE Status and Optical System Information visible).
inline void resetToBaselineState(ImGuiTestContext* ctx, TestAppContext* app) {
    ctx->PopupCloseAll();

    for (const auto& entry : DockableWindows) {
        const bool visible = app->windows->IsVisible(entry.id);
        const bool shouldBeVisible = (entry.id == WindowID::DDEStatus || entry.id == WindowID::SystemInfo);
        if (visible != shouldBeVisible) {
            clickMenuItem(ctx, ICON_FA_EYE " View", entry.name);
        }
    }
    ctx->Yield();
}

} // namespace app::testing
