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

#include "test_engine/test_engine.h"

#include "imgui_test_engine/imgui_te_context.h"
#include "imgui_test_engine/imgui_te_engine.h"

namespace app::testing {

/// Retrieve the shared context from a running test.
inline TestAppContext* appContext(ImGuiTestContext* ctx) {
    return static_cast<TestAppContext*>(ctx->Test->UserData);
}

/// The main menu bar lives in the "##MainMenuBar" window; its entries are
/// addressed as "//##MainMenuBar/##MenuBar/<menu>/<item>".
inline constexpr const char* kMenuBar = "//##MainMenuBar";

/// Build a path to an entry of the main menu bar.
///
/// The menu and item labels contain a FontAwesome icon in front of the visible
/// text (e.g. "\xEF\x81\xAE View"), and the test engine matches items by
/// hashing that exact label. The icon macros are therefore reused here so the
/// bytes are identical to the ones the application submits.
inline std::string menuPath(const char* menuLabelWithIcon, const char* itemLabel = nullptr) {
    std::string path = kMenuBar;
    path += "/##MenuBar/";
    path += menuLabelWithIcon;
    if (itemLabel) {
        path += '/';
        path += itemLabel;
    }
    return path;
}

/// The visible labels of the dock panels, as registered in DockableWindows.
inline const char* panelName(WindowID id) {
    for (const auto& entry : DockableWindows) {
        if (entry.id == id) {
            return entry.name;
        }
    }
    return nullptr;
}

/// Toggle a dock panel through the View menu and verify the visibility flag.
inline void togglePanel(ImGuiTestContext* ctx, DockableWindowsManager* windows,
                         WindowID id, const char* panelLabel) {
    const bool wasVisible = windows->IsVisible(id);

    ctx->ItemClick(menuPath(ICON_FA_EYE " View", panelLabel).c_str());
    IM_CHECK_NE(windows->IsVisible(id), wasVisible);

    ctx->ItemClick(menuPath(ICON_FA_EYE " View", panelLabel).c_str());
    IM_CHECK_EQ(windows->IsVisible(id), wasVisible);
}

/// Make sure no modal dialog is left open and all panels are back to the
/// default visibility (DDE Status and Optical System Information visible).
inline void resetToBaselineState(ImGuiTestContext* ctx, TestAppContext* app) {
    ctx->PopupCloseAll();

    for (const auto& entry : DockableWindows) {
        const bool visible = app->windows->IsVisible(entry.id);
        const bool shouldBeVisible = (entry.id == WindowID::DDEStatus || entry.id == WindowID::SystemInfo);
        if (visible != shouldBeVisible) {
            ctx->ItemClick(menuPath(ICON_FA_EYE " View", entry.name).c_str());
        }
    }
    ctx->Yield();
}

} // namespace app::testing
