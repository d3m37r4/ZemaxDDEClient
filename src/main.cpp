#include <fstream>

#include <imgui.h>

#include "logger/logger.h"
#include "app/app.h"

int main() {
    Logger logger;

    logger.addLog("[APP] Application started");

    AppContext* ctx = App::initialize(logger);
    if (!ctx) {
        logger.addLog("[APP] Application failed to initialize");
        return -1;
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
    App::shutdown(ctx);

    logger.addLog("[APP] Application terminated");
    return 0;
}
