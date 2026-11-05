#include <fstream>

#include <GLFW/glfw3.h>

#include "logger/logger.h"
#include "app/app.h"
#include "app/config_path.h"
#include "gui/gui.h"
#include "gui/dockable_windows_manager.h"
#include "gui/layout_presets.h"
#ifdef ZEMAX_ENABLE_TEST_ENGINE
#include "test_engine/test_cli.h"
#include "test_engine/test_engine.h"
#include "test_engine/test_profile.h"
#endif

namespace chrome = gui::chrome;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    Logger logger;

    logger.addLog("[APP] Application started");

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    const app::testing::TestCliOptions testOptions = app::testing::parseTestCliOptions(argc, argv);
    if (testOptions.isTestMode()) {
        // Redirect every persisted file into a throw-away directory so a test
        // run cannot touch the user's real profile.
        if (app::testing::useIsolatedTestProfile()) {
            logger.addLog("[TEST] Using isolated profile: " + app::testing::isolatedProfilePath());
        } else {
            logger.addLog("[TEST] WARNING: could not create an isolated profile, real config files may be modified");
        }
    }
#endif

    auto ctx = App::initialize(logger);
    if (!ctx) {
        logger.addLog("[APP] Application failed to initialize");
        return -1;
    }

    DockableWindowsManager wndMgr;
    wndMgr.RegisterDockableWindows(ctx->gui.get());

    const auto& general = ctx->gui->getSettingsManager().current().general;
    const auto& logging = ctx->gui->getSettingsManager().current().logging;
    // A saved default layout wins over the previous session; when it is absent
    // or was written by another ImGui build the regular restore path runs.
    const bool defaultLayoutApplied =
        general.applyDefaultLayoutOnStartup &&
        gui::layout::applySavedLayoutOnStartup(app::getDefaultLayoutPath(), wndMgr, logger);
    if (!defaultLayoutApplied) {
        if (general.restoreWindowLayout) {
            wndMgr.LoadState();
            // Minimized-window chrome state (tiles + floating rects) restores
            // under the same user opt-out as the rest of the window layout.
            chrome::loadState();
            // LoadState() has already restored the persisted Logs visibility
            // from windows.json; overriding it here would resurrect a window the
            // user closed via the View menu on the previous session.
        } else {
            wndMgr.SetVisible(WindowID::Logs, logging.showLogsOnStartup);
        }
    }
    // The Welcome card is startup-only and has no View menu entry: the
    // preference wins over whatever the restored layout had for it.
    wndMgr.SetVisible(WindowID::Welcome, general.showWelcomeOnStartup);

    ctx->gui->setWindowManager(&wndMgr);

    auto* menuBar = ctx->gui->getMenuBarController();
    if (menuBar) {
        menuBar->setWindowManager(&wndMgr);
    }

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    // The test engine must be started after ImGui::CreateContext() (done
    // inside GuiManager::initialize()).
    TestEngine testEngine;
    if (testOptions.isTestMode()) {
        testEngine.start(testOptions);
        app::testing::TestAppContext testAppContext;
        testAppContext.gui = ctx->gui.get();
        testAppContext.windows = &wndMgr;
        testEngine.registerTests(testAppContext);
        ctx->gui->setTestEngine(&testEngine);
    }
#endif

    logger.addLog("[APP] Main loop started");

    while (!ctx->gui->shouldClose()) {
        glfwPollEvents();

        ctx->gui->handleShortcuts();
        ctx->gui->render();

#ifdef ZEMAX_ENABLE_TEST_ENGINE
        testEngine.preSwap();
#endif
        glfwSwapBuffers(ctx->glfwWindow);
#ifdef ZEMAX_ENABLE_TEST_ENGINE
        testEngine.postSwap();

        // A test run ends by itself once the queue has drained.
        if (testEngine.isTestRunFinished()) {
            logger.addLog("[TEST] Test run finished");
            break;
        }
#endif
    }

    logger.addLog("[APP] Main loop ended");

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    // Stop the engine before the ImGui context is destroyed, and destroy the
    // engine context afterwards (the engine flushes its .ini data then).
    testEngine.stop();
#endif

    ctx->gui->getSettingsManager().saveToFile();
    wndMgr.SaveState();
    if (general.restoreWindowLayout) {
        // Keep the file consistent with the same user opt-out used on load;
        // an unchecked save would accumulate state that never restores.
        chrome::saveState();
    }

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    if (testOptions.isTestMode()) {
        testEngine.printResultSummary();
    }
#endif

    App::shutdown(*ctx);

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    testEngine.destroy();
    if (testOptions.isTestMode()) {
        app::testing::cleanupIsolatedTestProfile();
        return testEngine.exitCode();
    }
#endif

    logger.addLog("[APP] Application terminated");
    return 0;
}
