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
//
// Note: picking a theme radio button applies the theme immediately (live
// preview) but does not write it to the settings; only "Save" persists it.
// The tests therefore check the applied theme for the live preview and the
// persisted mode after saving.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

// Pick the theme radio button that differs from the currently applied theme
// and return the mode that was selected.
app::ThemeMode pickOppositeTheme(ImGuiTestContext* ctx, TestAppContext* app) {
    const bool isLight = appliedTheme(app) == kThemeNameLight;
    const app::ThemeMode mode = isLight ? app::ThemeMode::Dark : app::ThemeMode::Light;
    clickThemeRadio(ctx, mode);
    return mode;
}

} // namespace

void registerPreferencesTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Cancel discards the changes made in the dialog.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_cancel_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const std::string themeBefore = appliedTheme(a);
        const app::ThemeMode modeBefore = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx, a);
        const app::ThemeMode selected = pickOppositeTheme(ctx, a);
        IM_CHECK_NE(appliedTheme(a), themeBefore);

        clickDialogButton(ctx, "Preferences", ICON_FA_XMARK " Cancel");

        // The dialog closed and the working copy was discarded.
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
        IM_CHECK_EQ(appliedTheme(a), themeBefore);
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, modeBefore);
        IM_CHECK_NE(selected, modeBefore);
    };

    // X (the close button of the popup) behaves like Cancel.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_close_button_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const std::string themeBefore = appliedTheme(a);
        const app::ThemeMode modeBefore = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx, a);
        pickOppositeTheme(ctx, a);
        IM_CHECK_NE(appliedTheme(a), themeBefore);

        ctx->WindowClose("//Preferences");
        ctx->Yield();

        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
        IM_CHECK_EQ(appliedTheme(a), themeBefore);
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, modeBefore);
    };

    // Esc dismisses the dialog and discards the changes.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_escape_discards_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const std::string themeBefore = appliedTheme(a);
        const app::ThemeMode modeBefore = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx, a);
        pickOppositeTheme(ctx, a);
        IM_CHECK_NE(appliedTheme(a), themeBefore);

        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield();

        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
        IM_CHECK_EQ(appliedTheme(a), themeBefore);
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, modeBefore);
    };

    // Save applies and persists the changes.
    test = IM_REGISTER_TEST(engine, test_categories::kPreferences, "preferences_save_applies_changes");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode modeBefore = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx, a);
        const app::ThemeMode selected = pickOppositeTheme(ctx, a);

        clickDialogButton(ctx, "Preferences", ICON_FA_FLOPPY_DISK " Save");

        // The dialog closed and the new mode was persisted.
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, selected);
        IM_CHECK_NE(selected, modeBefore);

        // Restore the original value so the following tests start clean.
        openAppearanceSection(ctx, a);
        clickThemeRadio(ctx, modeBefore);
        clickDialogButton(ctx, "Preferences", ICON_FA_FLOPPY_DISK " Save");
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, modeBefore);
    };
}

} // namespace app::testing
