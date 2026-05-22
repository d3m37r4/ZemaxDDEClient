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

    void GraphicsBackend::initialize(GLFWwindow* window, Logger& logger, float initialDpiScale) {
        m_window = window;
        m_logger = &logger;

        if (!m_window) {
            throw std::runtime_error("Invalid GLFW window");
        }

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

        std::filesystem::path fontPath = std::filesystem::path{L"C:\\Windows\\Fonts"} / L"segoeui.ttf";
        const std::string fontPathStr = fontPath.string();

        float baseFontSize = BASE_FONT_SIZE;
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize);

        if (!font) {
            logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleDpi = dpiScale;
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(4.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 25.0f;
        style.ScrollbarSize = 15.0f;
        style.GrabMinSize = 10.0f;

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        updateDpiStyle(dpiScale);

        logger.addLog("[GUI] GUI initialized");
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
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
