// Screenshot capture of the application panels.
//
// The artifacts are meant for the README and for release notes, so each dock
// panel is captured on its own: the capture tool hides the other windows, which
// yields a clean image per panel.
//
// Files are written to <ZEMAX_TEST_SCREENSHOT_DIR> (or "output/captures" when
// the variable is not set). Video recording additionally requires ffmpeg and is
// therefore optional.

#include "test_engine/tests/test_common.h"

#include <cstdlib>
#include <filesystem>

namespace app::testing {

namespace {

std::string screenshotDir() {
    if (const char* env = std::getenv("ZEMAX_TEST_SCREENSHOT_DIR")) {
        if (env[0]) {
            return env;
        }
    }
    return "output/captures";
}

// Capture a single window; the capture tool hides every other window.
void captureWindow(ImGuiTestContext* ctx, const char* windowName) {
    ctx->CaptureReset();
    ctx->CaptureSetExtension(".png");
    ctx->CaptureScreenshotWindow(windowName);
    ctx->Yield();
}

} // namespace

void registerCaptureTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Capture a screenshot of every dock panel.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kCapture, "capture_panels");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        std::filesystem::path dir(screenshotDir());
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        // Make all panels visible so that each one can be captured.
        for (const auto& entry : DockableWindows) {
            if (!a->windows->IsVisible(entry.id)) {
                clickMenuItem(ctx, ICON_FA_EYE " View", entry.name);
            }
        }
        ctx->Yield();

        for (const auto& entry : DockableWindows) {
            IM_CHECK(a->windows->IsVisible(entry.id));
            captureWindow(ctx, entry.name);
        }
    };

    // Capture a screenshot of the modal dialogs. They carry their own
    // surface (popup background + inner child), so they are reviewed
    // separately from the dock panels.
    test = IM_REGISTER_TEST(engine, test_categories::kCapture, "capture_dialogs");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        std::filesystem::path dir(screenshotDir());
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        struct DialogEntry {
            const char* label;
            const char* windowName;
        };
        constexpr DialogEntry kDialogs[] = {
            {"About", "About"},
            {"Keyboard Shortcuts", "Keyboard Shortcuts"},
        };

        for (const auto& dialog : kDialogs) {
            clickMenuItem(ctx, ICON_FA_CIRCLE_QUESTION " Help", dialog.label);
            IM_CHECK(a->gui->isAnyModalOpen());
            captureWindow(ctx, dialog.windowName);
            clickDialogButton(ctx, dialog.windowName, "OK");
            IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);
        }
    };

    // Capture the panels in the light theme. The light theme has its own
    // surface ladder, so it needs its own screenshots. The theme is switched
    // through the theme manager directly: the dialog route would persist the
    // theme in the user settings.
    test = IM_REGISTER_TEST(engine, test_categories::kCapture, "capture_light");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        std::filesystem::path dir(screenshotDir());
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        // Make all panels visible so that each one can be captured.
        for (const auto& entry : DockableWindows) {
            if (!a->windows->IsVisible(entry.id)) {
                clickMenuItem(ctx, ICON_FA_EYE " View", entry.name);
            }
        }
        ctx->Yield();

        ThemeManager* themeManager = a->gui->getSettingsManager().getThemeManager();
        IM_CHECK(themeManager != nullptr);
        if (!themeManager) return;

        const std::string themeBefore = themeManager->currentThemeName();
        IM_CHECK(themeManager->apply(std::string{kThemeNameLight}));
        ctx->Yield();

        for (const auto& entry : DockableWindows) {
            captureWindow(ctx, entry.name);
        }

        // The About dialog carries its own surface, so it is captured too.
        clickMenuItem(ctx, ICON_FA_CIRCLE_QUESTION " Help", "About");
        IM_CHECK(a->gui->isAnyModalOpen());
        captureWindow(ctx, "About");
        clickDialogButton(ctx, "About", "OK");
        IM_CHECK_EQ(a->gui->isAnyModalOpen(), false);

        IM_CHECK(themeManager->apply(themeBefore));
        ctx->Yield();
    };
}

} // namespace app::testing
