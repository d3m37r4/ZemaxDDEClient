// Tests for the Preferences dialog dismissal contract.
//
// The contract (see commit "feat(ui): Save/Cancel/X/Esc dismissal contract
// for Preferences") is:
//   Save   -> values are applied and persisted, the dialog closes.
//   Cancel -> the working copy is discarded, the saved values stay untouched.
//   X / Esc-> the dialog closes and the working copy is discarded, exactly
//             like Cancel.
//
// These tests drive the real dialog through the real UI.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

const char* kLightRadio = ICON_FA_SUN " Light";
const char* kDarkRadio  = ICON_FA_MOON " Dark";

app::ThemeMode oppositeTheme(app::ThemeMode current) {
    switch (current) {
        case app::ThemeMode::Light: return app::ThemeMode::Dark;
        case app::ThemeMode::Dark:  return app::ThemeMode::Light;
        default:                     return app::ThemeMode::Dark;
    }
}

// Open the Preferences dialog through the File menu.
void openPreferences(ImGuiTestContext* ctx) {
    ctx->ItemClick(menuPath(ICON_FA_FOLDER_OPEN " File", ICON_FA_GEAR " Preferences").c_str());
    ctx->Yield();
}

// Pick the theme radio button that differs from the current mode.
void pickOppositeTheme(ImGuiTestContext* ctx, app::ThemeMode current) {
    ctx->ItemClick(current == app::ThemeMode::Light ? kDarkRadio : kLightRadio);
    ctx->Yield();
}

} // namespace

void registerPreferencesTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Cancel discards the changes made in the dialog.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_cancel_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;

        openPreferences(ctx);
        pickOppositeTheme(ctx, before);

        ctx->ItemClick(ICON_FA_XMARK " Cancel");
        ctx->Yield();

        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };

    // X (the close button of the popup) behaves like Cancel.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_close_button_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;

        openPreferences(ctx);
        pickOppositeTheme(ctx, before);

        // The X of a modal popup is the item registered under the popup name.
        ctx->ItemClick("//Preferences/Close");
        ctx->Yield();

        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };

    // Esc dismisses the dialog and discards the changes.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_escape_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;

        openPreferences(ctx);
        pickOppositeTheme(ctx, before);

        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield();

        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };

    // Save applies the changes.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_save_applies_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;
        const app::ThemeMode target = oppositeTheme(before);

        openPreferences(ctx);
        pickOppositeTheme(ctx, before);

        ctx->ItemClick(ICON_FA_FLOPPY_DISK " Save");
        ctx->Yield();

        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, target);

        // Restore the original value so the following tests start clean.
        openPreferences(ctx);
        pickOppositeTheme(ctx, target);
        ctx->ItemClick(ICON_FA_FLOPPY_DISK " Save");
        ctx->Yield();
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };
}

} // namespace app::testing
