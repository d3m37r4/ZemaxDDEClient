// Tests for the modal dialogs: About, Keyboard Shortcuts, Check for Updates.
//
// Each dialog is opened from the Help menu and must be dismissible again.
//
// Note: ImGui keeps the window object of a closed popup alive for a while, so
// the tests assert on GuiManager::isAnyModalOpen() rather than on the
// existence of the window.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

void openHelpDialog(ImGuiTestContext* ctx, const char* itemLabel) {
    clickMenuItem(ctx, ICON_FA_CIRCLE_QUESTION " Help", itemLabel);
}

} // namespace

void registerDialogTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // The About dialog opens and closes.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_about_opens_and_closes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        openHelpDialog(ctx, "About");
        IM_CHECK(a->gui->isAnyModalOpen());

        clickDialogButton(ctx, "About", "OK");
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
    };

    // The Keyboard Shortcuts popup opens and closes.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_shortcuts_open_and_close");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        openHelpDialog(ctx, "Keyboard Shortcuts");
        IM_CHECK(a->gui->isAnyModalOpen());

        clickDialogButton(ctx, "Keyboard Shortcuts", "OK");
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
    };

    // The Check for Updates popup opens and closes.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_updates_open_and_close");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        openHelpDialog(ctx, "Check for Updates");
        IM_CHECK(a->gui->isAnyModalOpen());

        clickDialogButton(ctx, "Check for Updates", "OK");
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
    };

    // Esc dismisses the topmost dialog.
    test = IM_REGISTER_TEST(engine, test_categories::kDialogs, "dialogs_escape_closes_dialog");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        openHelpDialog(ctx, "About");
        IM_CHECK(a->gui->isAnyModalOpen());

        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield();

        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
    };
}

} // namespace app::testing
