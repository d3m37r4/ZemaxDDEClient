#include <fstream>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "logger/logger.h"
#include "app/app.h"
#include "gui/dockable_windows_manager.h"

int main() {
    Logger logger;

    logger.addLog("[APP] Application started");

    auto ctx = App::initialize(logger);
    if (!ctx) {
        logger.addLog("[APP] Application failed to initialize");
        return -1;
    }

    DockableWindowsManager wndMgr;
    wndMgr.RegisterDockableWindows(ctx->gui.get());

    const auto& general = ctx->gui->getSettingsManager().current().general;
    if (general.restoreWindowLayout) {
        wndMgr.LoadState();
    }
    wndMgr.SetVisible(WindowID::DebugLog, general.showDebugLogOnStartup);

    ctx->gui->setWindowManager(&wndMgr);

    auto* menuBar = ctx->gui->getMenuBarController();
    if (menuBar) {
        menuBar->setWindowManager(&wndMgr);
    }

    logger.addLog("[APP] Main loop started");

    while (!ctx->gui->shouldClose()) {
        glfwPollEvents();

        // Hotkeys: Ctrl+O, Ctrl+,
        const bool ctrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
        if (ctrlDown && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            App::openZmxFileInZemax(logger);
        } else if (ctrlDown && ImGui::IsKeyPressed(ImGuiKey_Comma, false)) {
            if (menuBar) menuBar->openPreferences();
        }

        ctx->gui->render();
        glfwSwapBuffers(ctx->glfwWindow);
    }

    logger.addLog("[APP] Main loop ended");
    ctx->gui->getSettingsManager().saveToFile();
    wndMgr.SaveState();
    App::shutdown(*ctx);

    logger.addLog("[APP] Application terminated");
    return 0;
}
