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
            /// User font-scale multiplier over BASE_FONT_SIZE (Preferences → Appearance).
            /// Independent from DPI scaling; clamps to app::kMin/MaxFontScale.
            void setFontScale(float scale);
            void beginFrame();
            void endFrame();

            /// Updates the native Win11 title bar to match the current theme.
            /// Must be called whenever the theme changes at runtime.
            void updateTitleBarDarkMode(bool isDark);

            ThemeManager& getThemeManager() { return m_themeManager; }
            bool isLightTheme() const { return m_themeManager.isLight(); }
            ImVec4 getClearColor() const { return m_themeManager.getClearColor(); }

        private:
            /// Win32 polish for secondary viewport windows (see .cpp): sets
            /// GWL_HWNDPARENT ownership on secondary platform windows and
            /// raises them all when the main window regains focus from
            /// another application. Call after UpdatePlatformWindows().
            void syncPlatformWindows();

            GLFWwindow* m_window = nullptr;
            Logger* m_logger = nullptr;
            ThemeManager m_themeManager;
            /// HWND of the last GetForegroundWindow(), kept as void* so the
            /// header does not need windows.h.
            void* m_lastForegroundWindow = nullptr;
    };
}
