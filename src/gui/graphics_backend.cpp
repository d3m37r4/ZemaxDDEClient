#include <stdexcept>
#include <filesystem>
#include <format>
#include <algorithm>

#include <windows.h>

#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "gui/graphics_backend.h"
#include "gui/constants.h"
#include "app/config_path.h"
#include <cmath>
#include "logger/logger.h"

namespace gui {
    GraphicsBackend::~GraphicsBackend() {
        if (m_window) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot3D::DestroyContext();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
        }
    }

    void GraphicsBackend::initialize(GLFWwindow* window, Logger& logger, bool isLightTheme, float initialDpiScale) {
        m_window = window;
        m_logger = &logger;

        if (!m_window) {
            throw std::runtime_error("Invalid GLFW window");
        }

        // Register built-in themes
        m_themeManager.registerTheme(ThemeData::CreateWin11Light());
        m_themeManager.registerTheme(ThemeData::CreateWin11Dark());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImPlot3D::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = app::getImguiIniPath();

        float dpiScale = initialDpiScale;
        if (!std::isfinite(dpiScale) || dpiScale <= 0.0f) dpiScale = MIN_DPI_SCALE;
        dpiScale = std::clamp(dpiScale, MIN_DPI_SCALE, MAX_DPI_SCALE);

        io.DisplayFramebufferScale = ImVec2(dpiScale, dpiScale);
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        // Load Segoe UI with Cyrillic support
        {
            std::filesystem::path fontPath = std::filesystem::path{L"C:\\Windows\\Fonts"} / L"segoeui.ttf";
            const std::string fontPathStr = fontPath.string();

            float baseFontSize = BASE_FONT_SIZE;
            ImFontConfig fontCfg;
            fontCfg.OversampleH = 2;
            fontCfg.OversampleV = 1;
            fontCfg.PixelSnapH = true;
            ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize, &fontCfg, io.Fonts->GetGlyphRangesCyrillic());

            if (!font) {
                font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize);
            }

            if (!font) {
                logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
            }
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleDpi = dpiScale;

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        // Apply initial theme
        if (isLightTheme) {
            m_themeManager.apply(std::string{kThemeNameLight});
        } else {
            m_themeManager.apply(std::string{kThemeNameDark});
        }

        updateDpiStyle(dpiScale);

        logger.addLog(std::format("[GUI] GUI initialized (theme: {})", m_themeManager.currentThemeName()));
    }

    void GraphicsBackend::updateDpiStyle(float dpiScale) {
        dpiScale = std::clamp(dpiScale, MIN_DPI_SCALE, MAX_DPI_SCALE);

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleDpi = dpiScale;
    }

    void GraphicsBackend::beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GraphicsBackend::endFrame() {
        ImVec4 bg = m_themeManager.getClearColor();
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void GraphicsBackend::updateTitleBarDarkMode(bool isDark) {
        if (!m_window) return;

        #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
        #define DWMWA_USE_IMMERSIVE_DARK_MODE 19
        #endif

        HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
        if (dwmApi) {
            using DwmSetWindowAttributeFunc = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            auto* dwmFunc = reinterpret_cast<DwmSetWindowAttributeFunc>(
                reinterpret_cast<void*>(GetProcAddress(dwmApi, "DwmSetWindowAttribute")));

            if (dwmFunc) {
                BOOL useDarkMode = isDark;
                HWND hwnd = glfwGetWin32Window(m_window);
                if (hwnd) {
                    dwmFunc(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
                }
            }
            FreeLibrary(dwmApi);
        }
    }
}
