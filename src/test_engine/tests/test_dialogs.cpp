// Tests for the modal dialogs: About, Keyboard Shortcuts, Check for Updates.
//
// Each dialog is opened from the Help menu and must be dismissible again.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

void openHelpDialog(ImGuiTestContext* ctx, const char* itemLabel) {
    ctx->ItemClick(menuPath(ICON_FA_CIRCLE_QUESTION " Help", itemLabel).c_str());
    ctx->Yield();
}

} // namespace

void registerDialogTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // The About dialog opens and shows the application name.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_about_opens_and_closes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        openHelpDialog(ctx, "About");

        // The About popup must exist now.
        ImGuiTestItemInfo about = ctx->WindowInfo("//About");
        IM_CHECK(about.ID != 0);

        // The OK button closes it.
        ctx->ItemClick("//About/OK");
        ctx->Yield();

        IM_CHECK(ctx->WindowInfo("//About").ID == 0);
    };

    // The Keyboard Shortcuts popup opens and closes.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_shortcuts_open_and_close");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        openHelpDialog(ctx, "Keyboard Shortcuts");

        ImGuiTestItemInfo shortcuts = ctx->WindowInfo("//Keyboard Shortcuts");
        IM_CHECK(shortcuts.ID != 0);

        ctx->ItemClick("//Keyboard Shortcuts/OK");
        ctx->Yield();

        IM_CHECK(ctx->WindowInfo("//Keyboard Shortcuts").ID == 0);
    };

    // The Check for Updates popup opens and closes.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_updates_open_and_close");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        openHelpDialog(ctx, "Check for Updates");

        ImGuiTestItemInfo updates = ctx->WindowInfo("//Check for Updates");
        IM_CHECK(updates.ID != 0);

        ctx->ItemClick("//Check for Updates/OK");
        ctx->Yield();

        IM_CHECK(ctx->WindowInfo("//Check for Updates").ID == 0);
    };

    // Esc dismisses the topmost dialog.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_escape_closes_dialog");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        openHelpDialog(ctx, "About");
        IM_CHECK(ctx->WindowInfo("//About").ID != 0);

        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield();

        IM_CHECK(ctx->WindowInfo("//About").ID == 0);
    };
}

} // namespace app::testing
