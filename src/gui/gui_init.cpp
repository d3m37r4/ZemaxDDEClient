#include <stdexcept>
#include <windows.h>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "dde/dde_zemax_client.h"
#include "gui.h"

namespace gui {
    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient)
        : glfwWindow(glfwWindow)
        , hwndClient(hwndClient)
        , zemaxDDEClient(ddeClient)
        , dde_initialized(false)
        , selectedMenuItem(0)
        , show_updates_popup(false)
        , show_about_popup(false)
    {
        if (!glfwWindow) {
            throw std::runtime_error("Invalid GLFW window");
        }

        logger.addLog("(GuiManager) Received DDE client window handle = " + std::to_string((uintptr_t)hwndClient));
    }

    GuiManager::~GuiManager() {
        if (glfwWindow) {
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
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f);
        
        if (!font) {
            logger.addLog("(GUI MSG) Failed to load font segoeui.ttf");
        }

        // ImGui::StyleColorsClassic();
        // ImGui::StyleColorsLight();
        // ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        logger.addLog("(GUI MSG) GUI initialized");
    }
}