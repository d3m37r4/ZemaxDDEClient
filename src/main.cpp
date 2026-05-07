#include <fstream>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "logger/logger.h"
#include "app/app.h"
#include "gui/WindowManager.h"
#include "gui/WindowRegistration.h"

int main() {
    Logger logger;

    logger.addLog("[APP] Application started");

    auto ctx = App::initialize(logger);
    // Window manager for toggleable windows
    WindowManager wndMgr;
    RegisterAllWindows(wndMgr, ctx->gui.get());
    wndMgr.LoadState();

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

        // Tools menu with window toggles
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Tools")) {
                for (auto &pair : wndMgr.GetVisibilities()) {
                    bool visible = pair.second;
                    const char* name = wndMgr.GetNames().at(pair.first).c_str();
                    if (ImGui::MenuItem(name, nullptr, &visible))
                        wndMgr.SetVisible(pair.first, visible);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Render toggleable windows
        wndMgr.RenderAll();

        ctx->gui->render();
        glfwSwapBuffers(ctx->glfwWindow);
    }

    logger.addLog("[APP] Main loop ended");
    wndMgr.SaveState();
    App::shutdown(*ctx);

    logger.addLog("[APP] Application terminated");
    return 0;
}
