#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "gui/theme_manager.h"

class Logger;

namespace gui {
    class GraphicsBackend {
        public:
            GraphicsBackend() = default;
            ~GraphicsBackend();

            GraphicsBackend(const GraphicsBackend&) = delete;
            GraphicsBackend& operator=(const GraphicsBackend&) = delete;

            void initialize(GLFWwindow* window, Logger& logger, bool isLightTheme = true, float initialDpiScale = 1.0f);
            void updateDpiStyle(float dpiScale);
            void beginFrame();
            void endFrame();

            void applyTheme(bool isLight);
            void toggleTheme();
            ThemeManager& getThemeManager() { return m_themeManager; }
            bool isLightTheme() const { return m_themeManager.isLight(); }
            ImVec4 getClearColor() const { return m_themeManager.getClearColor(); }

        private:
            GLFWwindow* m_window = nullptr;
            Logger* m_logger = nullptr;
            ThemeManager m_themeManager;
    };
}
