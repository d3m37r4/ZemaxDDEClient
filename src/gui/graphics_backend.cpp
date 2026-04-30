#include <stdexcept>
#include <filesystem>

#include <windows.h>

#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"

#include "gui/graphics_backend.h"
#include <cmath>
#include "logger/logger.h"

namespace gui {
    GraphicsBackend::~GraphicsBackend() {
        if (m_window) {
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
        }
    }

    void GraphicsBackend::initialize(GLFWwindow* window, Logger& logger, const char* iniPath, float initialDpiScale) {
        m_window = window;

        if (!m_window) {
            throw std::runtime_error("Invalid GLFW window");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = iniPath;

        // Use pre-computed DPI scale passed from app initialization
        float dpiScale = initialDpiScale;
        if (!std::isfinite(dpiScale)) dpiScale = 1.0f;
        if (dpiScale <= 0.0f) dpiScale = 0.01f;
        if (dpiScale > 98.0f) dpiScale = 98.0f;
        io.DisplayFramebufferScale = ImVec2(dpiScale, dpiScale);

        std::filesystem::path fontPath = std::filesystem::path{L"C:\\Windows\\Fonts"} / L"segoeui.ttf";
        const std::string fontPathStr = fontPath.string();

        // Load font at base size, then scale via FontGlobalScale
        float baseFontSize = 18.0f;
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize);

        if (!font) {
            logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
        }

        ImGuiStyle& style = ImGui::GetStyle();
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
        // Clamp to valid range to avoid ImGui assertion failures
        if (dpiScale <= 0.0f) dpiScale = 0.01f;
        if (dpiScale > 98.0f) dpiScale = 98.0f;
        ImGuiIO& io = ImGui::GetIO();

        // Only scale font globally, not styles (they're already scaled at init)
        io.FontGlobalScale = dpiScale;
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
