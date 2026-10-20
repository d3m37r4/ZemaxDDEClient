// Screenshot capture of the application panels.
//
// The artifacts are meant for the README and for release notes, so they are
// written to a directory next to the executable (or to the path given with
// ZEMAX_TEST_SCREENSHOT_DIR). Video recording additionally requires ffmpeg
// and is therefore optional.

#include "test_engine/tests/test_common.h"

#include <cstdlib>
#include <filesystem>

namespace app::testing {

namespace {

// Directory the screenshots are written to.
std::string screenshotDir() {
    if (const char* env = std::getenv("ZEMAX_TEST_SCREENSHOT_DIR")) {
        if (env[0]) {
            return env;
        }
    }
    return "test_screenshots";
}

void captureScreenshot(ImGuiTestContext* ctx, const char* name) {
    std::filesystem::path dir(screenshotDir());
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    ctx->CaptureReset();
    ctx->CaptureSetExtension(".png");
    // Capture the whole application window (all visible panels).
    ctx->CaptureScreenshot(ImGuiCaptureFlags_IncludeOtherWindows | ImGuiCaptureFlags_IncludePopups);
    ctx->Yield();
}

} // namespace

void registerCaptureTests(ImGuiTestEngine* engine, TestAppContext* app) {
    // Capture a screenshot with every dock panel visible.
    ImGuiTest* test = IM_REGISTER_TEST(engine, test_categories::kCapture, "capture_panels");
    test->UserData = app;
    test->TestFunc = [](ImGuiTestContext* ctx) {
        TestAppContext* a = appContext(ctx);

        // Make all panels visible so the screenshot shows the full UI.
        for (const auto& entry : DockableWindows) {
            if (!a->windows->IsVisible(entry.id)) {
                ctx->ItemClick(menuPath(ICON_FA_EYE " View", entry.name).c_str());
            }
        }
        ctx->Yield();

        captureScreenshot(ctx, "all_panels");
    };
}

} // namespace app::testing
