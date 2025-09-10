#include <imgui.h>
#include "logger/logger.h"
#include "app/app_init.h"
#include "application.h"

Logger logger;

extern "C" LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    if (iMsg >= WM_DDE_FIRST && iMsg <= WM_DDE_LAST) {
        ZemaxDDE::ZemaxDDEClient* client = (ZemaxDDE::ZemaxDDEClient*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (client) {
            return client->handleDDEMessages(iMsg, wParam, lParam);
        } else {
        #ifdef DEBUG_LOG
            logger.addLog("[APP] WndProc: No ZemaxDDEClient instance associated with hwnd = " + std::to_string((uintptr_t)hwnd));
        #endif
        }
    }
    return DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

int main() {
    logger.addLog("[APP] Application started");

    AppContext* ctx = initializeApplication();
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
                Application::openZmxFileInZemax();
            }
        }

        ctx->gui->render();
        glfwSwapBuffers(ctx->glfwWindow);
    }

    logger.addLog("[APP] Main loop ended");
    shutdownApplication(ctx);

    logger.addLog("[APP] Application terminated");
    return 0;
}
