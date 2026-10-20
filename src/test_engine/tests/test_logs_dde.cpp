// Tests for the Logs panel and the DDE Status panel in the disconnected state.
//
// Zemax is not required for these tests: the application is expected to work
// (and to report "not connected") when no DDE server is available.

#include "test_engine/tests/test_common.h"

namespace app::testing {

void registerLogsDdeTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // The Logs panel shows the application log entries.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kLogsDde, "logs_panel_shows_entries");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        if (!a->windows->IsVisible(WindowID::Logs)) {
            ctx->ItemClick(menuPath(ICON_FA_EYE " View", "Logs").c_str());
            ctx->Yield();
        }
        IM_CHECK(a->windows->IsVisible(WindowID::Logs));

        // The window exists and has content.
        ImGuiTestItemInfo logs = ctx->WindowInfo("//Logs");
        IM_CHECK(logs.ID != 0);
    };

    // Without a Zemax instance the DDE panel must show a disconnected state.
    test = IM_REGISTER_TEST(engine, test_categories::kLogsDde, "dde_panel_reports_disconnected");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        if (!a->windows->IsVisible(WindowID::DDEStatus)) {
            ctx->ItemClick(menuPath(ICON_FA_EYE " View", "DDE Status").c_str());
            ctx->Yield();
        }
        IM_CHECK(a->windows->IsVisible(WindowID::DDEStatus));

        ImGuiTestItemInfo status = ctx->WindowInfo("//DDE Status");
        IM_CHECK(status.ID != 0);
    };

    // The DDE menu shows the connect entry.
    test = IM_REGISTER_TEST(engine, test_categories::kLogsDde, "dde_menu_connect_entry_present");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        // "Connect to Zemax..." is always available (a free slot exists).
        ImGuiTestItemInfo connect =
            ctx->ItemInfo(menuPath(ICON_FA_COMPUTER " DDE", "Connect to Zemax...").c_str());
        IM_CHECK(connect.ID != 0);

        // "Disconnect Active Slot" exists but is not checked while nothing is
        // connected.
        ImGuiTestItemInfo disconnect =
            ctx->ItemInfo(menuPath(ICON_FA_COMPUTER " DDE", "Disconnect Active Slot").c_str());
        IM_CHECK(disconnect.ID != 0);
        IM_CHECK((disconnect.StatusFlags & ImGuiItemStatusFlags_Checked) == 0);
    };
}

} // namespace app::testing
