// Smoke tests: the application starts, its panels can be toggled from the
// View menu and with the Ctrl+1..5 shortcuts, and the dock panels show up.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

// Panel -> shortcut key, mirroring the Ctrl+1..5 bindings.
struct PanelEntry {
    WindowID id;
    const char* name;
    ImGuiKey shortcut;
};

constexpr PanelEntry kPanels[] = {
    { WindowID::DDEStatus,               "DDE Status",                 ImGuiKey_1 },
    { WindowID::SystemInfo,              "Optical System Information", ImGuiKey_2 },
    { WindowID::SurfaceProfileInspector, "Surface Profile Inspector",  ImGuiKey_3 },
    { WindowID::SurfaceIrregularityMap,  "Surface Irregularity Map",    ImGuiKey_4 },
    { WindowID::Logs,                    "Logs",                       ImGuiKey_5 },
};

} // namespace

void registerSmokeTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Every panel can be shown and hidden through the View menu.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kSmoke, "smoke_view_menu_toggles_panels");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        for (const PanelEntry& panel : kPanels) {
            togglePanel(ctx, a->windows, panel.id, panel.name);
        }
    };

    // The Ctrl+1..5 shortcuts toggle the same panels.
    test = IM_REGISTER_TEST(engine, test_categories::kSmoke, "smoke_shortcuts_toggle_panels");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        for (const PanelEntry& panel : kPanels) {
            const bool wasVisible = a->windows->IsVisible(panel.id);

            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->KeyPress(panel.shortcut);
            ctx->KeyUp(ImGuiMod_Ctrl);
            IM_CHECK_NE(a->windows->IsVisible(panel.id), wasVisible);

            ctx->KeyDown(ImGuiMod_Ctrl);
            ctx->KeyPress(panel.shortcut);
            ctx->KeyUp(ImGuiMod_Ctrl);
            IM_CHECK_EQ(a->windows->IsVisible(panel.id), wasVisible);
        }
    };

    // Toggling a panel actually creates (and destroys) the dock window.
    test = IM_REGISTER_TEST(engine, test_categories::kSmoke, "smoke_panel_window_appears");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        // Open the Logs panel and make sure its window exists.
        if (!a->windows->IsVisible(WindowID::Logs)) {
            ctx->ItemClick(menuPath(ICON_FA_EYE " View", "Logs").c_str());
            ctx->Yield();
        }
        IM_CHECK(a->windows->IsVisible(WindowID::Logs));

        // The window itself must be reachable by name.
        ImGuiTestItemInfo window = ctx->WindowInfo("//Logs");
        IM_CHECK(window.ID != 0);
    };

    // Every panel window can be opened without crashing. This is the
    // "open all tools" smoke test.
    test = IM_REGISTER_TEST(engine, test_categories::kSmoke, "smoke_open_all_panels");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        for (const PanelEntry& panel : kPanels) {
            if (!a->windows->IsVisible(panel.id)) {
                ctx->ItemClick(menuPath(ICON_FA_EYE " View", panel.name).c_str());
            }
            IM_CHECK(a->windows->IsVisible(panel.id));
            // Give the window a frame to appear.
            ctx->Yield();
        }

        // All of them are open now.
        for (const PanelEntry& panel : kPanels) {
            IM_CHECK(a->windows->IsVisible(panel.id));
        }
    };
}

} // namespace app::testing
