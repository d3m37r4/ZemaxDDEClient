#include <fstream>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "logger/logger.h"
#include "app/app.h"
#include "gui/window_manager.h"
#include "gui/window_registration.h"

int main() {
    Logger logger;

    logger.addLog("[APP] Application started");

    auto ctx = App::initialize(logger);
    if (!ctx) {
        logger.addLog("[APP] Application failed to initialize");
        return -1;
    }

    WindowManager wndMgr;
    RegisterAllWindows(wndMgr, ctx->gui.get());
    wndMgr.LoadState();
    ctx->gui->setWindowManager(&wndMgr);

    auto* menuBar = ctx->gui->getMenuBarController();
    if (menuBar) {
        menuBar->setWindowManager(&wndMgr);
    }

    logger.addLog("[APP] Main loop started");

    while (!ctx->gui->shouldClose()) {
        glfwPollEvents();

        // Hotkey Ctrl+O
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
            if (ImGui::IsKeyPressed(ImGuiKey_O)) {
                App::openZmxFileInZemax(logger);
            }
        }

        ctx->gui->render();
        glfwSwapBuffers(ctx->glfwWindow);
    }

    logger.addLog("[APP] Main loop ended");
    wndMgr.SaveState();
    App::shutdown(*ctx);

    logger.addLog("[APP] Application terminated");
    return 0;
}
