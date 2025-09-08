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
            logger.addLog("WndProc: No ZemaxDDEClient instance associated with hwnd = " + std::to_string((uintptr_t)hwnd));
        }
    }
    return DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

int main() {
    AppContext* ctx = initializeApplication();
    if (!ctx) return -1;

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

    shutdownApplication(ctx);
    return 0;
}
