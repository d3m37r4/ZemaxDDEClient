#include <stdexcept>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "app/app.h"
#include "gui.h"

namespace {
    constexpr std::string_view IMGUI_INI_FILENAME = "imgui.ini";

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

                std::wstring imguiIniWidePath = configFolderPath + L"\\" + std::wstring(IMGUI_INI_FILENAME.begin(), IMGUI_INI_FILENAME.end());

                int utf8PathLength = WideCharToMultiByte(CP_UTF8, 0, imguiIniWidePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (utf8PathLength > 0) {
                    imguiIniPath.resize(utf8PathLength - 1);
                    WideCharToMultiByte(CP_UTF8, 0, imguiIniWidePath.c_str(), -1, &imguiIniPath[0], utf8PathLength, nullptr, nullptr);
                #ifdef DEBUG_LOG
                    logger.addLog(std::format("[GUI] ImGui INI path set to: {}", imguiIniPath));
                #endif
                } else {
                    imguiIniPath = std::string(IMGUI_INI_FILENAME);
                    logger.addLog(std::format("[GUI] Warning: Failed to convert ImGui INI path to UTF-8. Using fallback: {}", imguiIniPath));
                }
            } else {
                imguiIniPath = std::string(IMGUI_INI_FILENAME);
                logger.addLog(std::format("[GUI] Warning: Failed to get %LOCALAPPDATA%. Using fallback path: {}", imguiIniPath));
            }
        #else
            imguiIniPath = std::string(IMGUI_INI_FILENAME);
            logger.addLog(std::format("[GUI] Development mode: ImGui INI path = {}", imguiIniPath));
        #endif
        }

        return imguiIniPath.c_str();
    }
}

namespace gui {
    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient)
        : m_glfwWindow(glfwWindow)
        , m_hwndClient(hwndClient)
        , m_zemaxDDEClient(ddeClient)
        , m_showUpdatesPopup(false)
        , m_showAboutPopup(false)
    {
        if (!m_glfwWindow) {
            throw std::runtime_error("Invalid GLFW window");
        }

        logger.addLog(std::format("[GUI] Received DDE client window handle = {}", reinterpret_cast<uintptr_t>(hwndClient)));
    }

    GuiManager::~GuiManager() {
        if (m_glfwWindow) {
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

        // Get DPI scale factor for font and UI scaling
        HDC hdc = GetDC(NULL);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        float dpiScale = static_cast<float>(dpiX) / 96.0f;

        std::filesystem::path fontPath = std::filesystem::path{L"C:\\Windows\\Fonts"} / L"segoeui.ttf";
        const std::string fontPathStr = fontPath.string();

        // Load font already scaled to DPI — do NOT use FontGlobalScale (it breaks input field text rendering)
        float baseFontSize = 18.0f;
        float scaledFontSize = baseFontSize * dpiScale;
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), scaledFontSize);

        if (!font) {
            logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.0f * dpiScale, 8.0f * dpiScale);
        style.FramePadding = ImVec2(4.0f * dpiScale, 4.0f * dpiScale);
        style.ItemSpacing = ImVec2(8.0f * dpiScale, 4.0f * dpiScale);
        style.ItemInnerSpacing = ImVec2(4.0f * dpiScale, 4.0f * dpiScale);
        style.IndentSpacing = 25.0f * dpiScale;
        style.ScrollbarSize = 15.0f * dpiScale;
        style.GrabMinSize = 10.0f * dpiScale;

        ImGui_ImplGlfw_InitForOpenGL(m_glfwWindow, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        logger.addLog("[GUI] GUI initialized");
    }

    void GuiManager::updateDpiStyle(float dpiScale) {
        ImGuiIO& io = ImGui::GetIO();

        // Scale font globally (fonts already loaded)
        io.FontGlobalScale = dpiScale;

        // Scale ImGui style metrics
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.0f * dpiScale, 8.0f * dpiScale);
        style.FramePadding = ImVec2(4.0f * dpiScale, 4.0f * dpiScale);
        style.ItemSpacing = ImVec2(8.0f * dpiScale, 4.0f * dpiScale);
        style.ItemInnerSpacing = ImVec2(4.0f * dpiScale, 4.0f * dpiScale);
        style.IndentSpacing = 25.0f * dpiScale;
        style.ScrollbarSize = 15.0f * dpiScale;
        style.GrabMinSize = 10.0f * dpiScale;
    }
}
