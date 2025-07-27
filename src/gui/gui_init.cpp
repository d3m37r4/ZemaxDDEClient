#include <stdexcept>
#include <windows.h>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "dde_client.h"
#include "gui.h"

namespace gui {
    GuiManager::GuiManager(GLFWwindow* win, HWND hwndDDE)
        : window(win)
        , hwndDDE(hwndDDE)
        , dde_initialized(false)
        , selectedMenuItem(0)
        , surface_number(1)
        , radius(0.0f)
        , show_about_popup(false)
        , show_updates_popup(false)
        , logger()
    {
        if (!window) throw std::runtime_error("Invalid GLFW window");
        errorMsg[0] = '\0';
        logger.addLog("GuiManager created with hwndDDE: " + std::to_string((uintptr_t)hwndDDE));
    }

    GuiManager::~GuiManager() {
        if (window) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
    }

    void GuiManager::initialize() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        char fontPath[MAX_PATH];
        GetWindowsDirectoryA(fontPath, MAX_PATH);
        strcat_s(fontPath, "\\Fonts\\segoeui.ttf");
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 17.0f);
        if (!font) logger.addLog("Failed to load font segoeui.ttf");

        // ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        logger.addLog("GUI initialized");
    }
}