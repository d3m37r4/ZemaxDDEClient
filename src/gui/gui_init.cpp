#include <stdexcept>
#include <fstream>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "app/app.h"
#include "gui.h"

namespace {
    const char* imguiIniFilename = "imgui.ini";

    const char* getImGuiIniPath() {
        static std::string imguiIniPath;

        if (imguiIniPath.empty()) {
        #ifdef APP_PRODUCTION_BUILD
            PWSTR localAppDataPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataPath))) {
                const char* appNameUtf8 = APP_NAME;
                int appNameWideLength = MultiByteToWideChar(CP_UTF8, 0, appNameUtf8, -1, nullptr, 0);
                std::wstring appNameWide(appNameWideLength - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, appNameUtf8, -1, &appNameWide[0], appNameWideLength);

                std::wstring configFolderPath = std::wstring(localAppDataPath) + L"\\" + appNameWide;
                CoTaskMemFree(localAppDataPath);
                std::filesystem::create_directories(configFolderPath);

                std::wstring imguiIniWidePath = configFolderPath + L"\\" + std::wstring(imguiIniFilename, imguiIniFilename + strlen(imguiIniFilename));

                int utf8PathLength = WideCharToMultiByte(CP_UTF8, 0, imguiIniWidePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (utf8PathLength > 0) {
                    imguiIniPath.resize(utf8PathLength - 1);
                    WideCharToMultiByte(CP_UTF8, 0, imguiIniWidePath.c_str(), -1, &imguiIniPath[0], utf8PathLength, nullptr, nullptr);
                #ifdef DEBUG_LOG
                    logger.addLog(std::format("[GUI] ImGui INI path set to: {}", imguiIniPath));
                #endif
                } else {
                    imguiIniPath = imguiIniFilename;
                    logger.addLog(std::format("[GUI] Warning: Failed to convert ImGui INI path to UTF-8. Using fallback: {}", imguiIniPath));
                }
            } else {
                imguiIniPath = imguiIniFilename;
                logger.addLog(std::format("[GUI] Warning: Failed to get %LOCALAPPDATA%. Using fallback path: {}", imguiIniPath));
            }
        #else
            imguiIniPath = imguiIniFilename;
            logger.addLog(std::format("[GUI] Development mode: ImGui INI path = {}", imguiIniPath));
        #endif
        }

        return imguiIniPath.c_str();
    }
}

namespace gui {
    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient)
        : glfwWindow(glfwWindow)
        , hwndClient(hwndClient)
        , zemaxDDEClient(ddeClient)
        , show_updates_popup(false)
        , show_about_popup(false)
    {
        if (!glfwWindow) {
            throw std::runtime_error("Invalid GLFW window");
        }

        logger.addLog(std::format("[GUI] Received DDE client window handle = {}", (uintptr_t)hwndClient));
    }

    GuiManager::~GuiManager() {
        if (glfwWindow) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
        }
    }

    void GuiManager::initialize() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = getImGuiIniPath();

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