#pragma once

#include <GLFW/glfw3.h>

class Logger;

namespace gui {
    /**
     * @brief Manages ImGui + ImPlot + OpenGL backend lifecycle.
     *        Handles initialization, DPI scaling, and shutdown.
     */
    class GraphicsBackend {
        public:
            GraphicsBackend() = default;
            ~GraphicsBackend();

            // Non-copyable
            GraphicsBackend(const GraphicsBackend&) = delete;
            GraphicsBackend& operator=(const GraphicsBackend&) = delete;

            void initialize(GLFWwindow* window, Logger& logger, const char* iniPath);
            void updateDpiStyle(float dpiScale);
            void beginFrame();
            void endFrame();

        private:
            GLFWwindow* m_window = nullptr;
    };
}
