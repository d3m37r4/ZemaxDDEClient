// Tests for the theme switching in the Preferences dialog.
//
// The Appearance section of the dialog offers Light / Dark / System. Picking
// one applies it immediately (live preview) and Save persists it, while
// Cancel / X / Esc revert to the previous value.

#include "test_engine/tests/test_common.h"

namespace app::testing {

namespace {

// Open Preferences and switch to the Appearance section.
void openAppearanceSection(ImGuiTestContext* ctx) {
    ctx->ItemClick(menuPath(ICON_FA_FOLDER_OPEN " File", ICON_FA_GEAR " Preferences").c_str());
    ctx->Yield();

    // The Appearance entry lives in the sidebar of the dialog.
    ctx->ItemClick("//Preferences/Appearance");
    ctx->Yield();
}

} // namespace

void registerThemeTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Picking a radio button changes the applied theme right away.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kTheme, "theme_radio_button_applies_immediately");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx);

        // Click the radio button that differs from the current mode.
        const char* radio = (before == app::ThemeMode::Light) ? ICON_FA_MOON " Dark" : ICON_FA_SUN " Light";
        ctx->ItemClick(radio);
        ctx->Yield();

        // The live preview changed the effective theme.
        IM_CHECK_NE(a->gui->getSettingsManager().current().appearance.themeMode, before);

        // Cancel restores the original value.
        ctx->ItemClick(ICON_FA_XMARK " Cancel");
        ctx->Yield();
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };

    // All three theme modes can be selected.
    test = IM_REGISTER_TEST(engine, test_categories::kTheme, "theme_all_modes_selectable");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);
        const app::ThemeMode before = a->gui->getSettingsManager().current().appearance.themeMode;

        openAppearanceSection(ctx);

        // Every radio button must be present.
        IM_CHECK(ctx->ItemInfo(ICON_FA_SUN " Light").ID != 0);
        IM_CHECK(ctx->ItemInfo(ICON_FA_MOON " Dark").ID != 0);
        IM_CHECK(ctx->ItemInfo(ICON_FA_DISPLAY " System").ID != 0);

        ctx->ItemClick(ICON_FA_DISPLAY " System");
        ctx->Yield();
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, app::ThemeMode::System);

        // Restore.
        ctx->ItemClick(ICON_FA_XMARK " Cancel");
        ctx->Yield();
        IM_CHECK_EQ(a->gui->getSettingsManager().current().appearance.themeMode, before);
    };
}

} // namespace app::testing
