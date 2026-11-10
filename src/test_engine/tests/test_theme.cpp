// Tests for the theme switching in the Preferences dialog.
//
// The Appearance section of the dialog offers Light / Dark / System. Picking
// one applies it immediately (live preview); only "Save" persists it, while
// Cancel / X / Esc revert to the previous value.

#include "test_engine/tests/test_common.h"

namespace app::testing {

void registerThemeTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Picking a radio button changes the applied theme right away.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kTheme, "theme_radio_button_applies_immediately");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const std::string themeBefore = appliedTheme(a);

        openAppearanceSection(ctx, a);

        // Click the radio button that differs from the currently applied theme.
        const bool isLight = themeBefore == kThemeNameLight;
        const app::ThemeMode mode = isLight ? app::ThemeMode::Dark : app::ThemeMode::Light;
        clickThemeRadio(ctx, mode);

        // The live preview changed the effective theme.
        IM_CHECK_EQ(appliedTheme(a), themeNameOf(mode));

        // Cancel restores the original theme.
        clickDialogButton(ctx, "Preferences", ICON_FA_XMARK " Cancel");
        IM_CHECK_EQ(appliedTheme(a), themeBefore);
    };

    // All three theme modes can be selected.
    test = IM_REGISTER_TEST(engine, test_categories::kTheme, "theme_all_modes_selectable");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const std::string themeBefore = appliedTheme(a);
        const app::ThemeMode modeBefore = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx, a);
        setRefToChildWindow(ctx, "Preferences", "##prefs_content");

        // Every radio button must be present.
        IM_CHECK(ctx->ItemInfo(themeRadioLabel(app::ThemeMode::Light)).ID != 0);
        IM_CHECK(ctx->ItemInfo(themeRadioLabel(app::ThemeMode::Dark)).ID != 0);
        IM_CHECK(ctx->ItemInfo(themeRadioLabel(app::ThemeMode::System)).ID != 0);

        // Select Dark and make sure it is applied.
        clickThemeRadio(ctx, app::ThemeMode::Dark);
        IM_CHECK_EQ(appliedTheme(a), themeNameOf(app::ThemeMode::Dark));

        // Select System: the resolved theme then follows the OS setting, so
        // only the persisted mode is checked after saving.
        clickThemeRadio(ctx, app::ThemeMode::System);
        clickDialogButton(ctx, "Preferences", ICON_FA_FLOPPY_DISK " Save");
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, app::ThemeMode::System);

        // Restore the original mode.
        openAppearanceSection(ctx, a);
        clickThemeRadio(ctx, modeBefore);
        clickDialogButton(ctx, "Preferences", ICON_FA_FLOPPY_DISK " Save");
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, modeBefore);
        IM_CHECK_EQ(appliedTheme(a), themeBefore);
    };
}

} // namespace app::testing
