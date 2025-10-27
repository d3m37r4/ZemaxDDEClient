#include "gui.h"

namespace gui {
    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient)
        : glfwWindow(glfwWindow)
        , hwndClient(hwndClient)
        , zemaxDDEClient(ddeClient)
        , selectedMenuItem(0)
        , show_updates_popup(false)
        , show_about_popup(false)
    {
        if (!glfwWindow) {
            throw std::runtime_error("Invalid GLFW window");
        }

        logger.addLog("[GUI] Received DDE client window handle = " + std::to_string((uintptr_t)hwndClient));
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
            logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
        }

        // ImGui::StyleColorsClassic();
        // ImGui::StyleColorsLight();
        // ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        logger.addLog("[GUI] GUI initialized");
    }
}