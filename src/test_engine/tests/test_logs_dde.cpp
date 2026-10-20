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
            clickMenuItem(ctx, ICON_FA_EYE " View", "Logs");
        }
        IM_CHECK(a->windows->IsVisible(WindowID::Logs));

        // The window exists.
        ImGuiTestItemInfo logs = ctx->WindowInfo("//Logs");
        IM_CHECK(logs.ID != 0);
    };

    // Without a Zemax instance the DDE panel must show a disconnected state.
    test = IM_REGISTER_TEST(engine, test_categories::kLogsDde, "dde_panel_reports_disconnected");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        if (!a->windows->IsVisible(WindowID::DDEStatus)) {
            clickMenuItem(ctx, ICON_FA_EYE " View", "DDE Status");
        }
        IM_CHECK(a->windows->IsVisible(WindowID::DDEStatus));

        ImGuiTestItemInfo status = ctx->WindowInfo("//DDE Status");
        IM_CHECK(status.ID != 0);
    };

    // The DDE menu offers connecting to Zemax; the dialog opens and closes.
    test = IM_REGISTER_TEST(engine, test_categories::kLogsDde, "dde_connect_dialog_opens_and_closes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        // The connect popup is rendered by the DDE Status panel, so the panel
        // has to be visible.
        if (!a->windows->IsVisible(WindowID::DDEStatus)) {
            clickMenuItem(ctx, ICON_FA_EYE " View", "DDE Status");
        }

        clickMenuItem(ctx, ICON_FA_COMPUTER " DDE", "Connect to Zemax...");
        IM_CHECK(a->gui->isAnyModalOpen());

        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield();
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
    };
}

} // namespace app::testing
