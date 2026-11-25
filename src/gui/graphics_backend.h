#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <vector>

#include "gui/theme_manager.h"
#include "gui/viewport_drag_snap.h"

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
            /// Interface font file (empty = built-in default). Rebuilds the font
            /// atlas; the icon font is merged again, so icons keep working.
            void setFontPath(const std::string& path);
            void beginFrame();
            /// @param anyModalOpen While a modal dialog is open the secondary
            /// platform windows lose their ownership for that time (see
            /// syncPlatformWindows), letting the main window's popups cover them.
            void endFrame(bool anyModalOpen = false);

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
            /// another application. While @p anyModalOpen the ownership is
            /// released and the panels sink directly below the main window;
            /// on close they are inserted back just above it. Our windows
            /// never cross the z-order of unrelated applications.
            /// Call after UpdatePlatformWindows().
            void syncPlatformWindows(bool anyModalOpen);
            /// (Re)builds the atlas: base font (Cyrillic ranges) plus the merged
            /// Font Awesome icon font.
            void loadFonts(const std::string& requestedPath);

            GLFWwindow* m_window = nullptr;
            Logger* m_logger = nullptr;
            ThemeManager m_themeManager;
            /// HWND of the last GetForegroundWindow(), kept as void* so the
            /// header does not need windows.h.
            void* m_lastForegroundWindow = nullptr;
            /// Modal band state: true while the ownership is released for a
            /// modal popup; our panels' z-order captured at the rising edge
            /// (HWNDs as void* so the header does not need windows.h).
            bool m_modalOwnershipActive = false;
            std::vector<void*> m_modalPanelOrder;
            /// Secondary viewports already given the Win32 polish (tool-window
            /// style + DWM rounding). Stored as void* per the header rule about
            /// not including windows.h here.
            std::vector<void*> m_preparedViewports;
            /// Interface font currently requested by the settings ("" = default).
            std::string m_fontPath;
            /// Font change requested during a frame; applied before the next
            /// NewFrame(), where no ImFont is in use any more.
            std::string m_pendingFontPath;
            bool m_fontReloadPending = false;
            /// Sends a secondary window dragged to a monitor edge into the same
            /// half or quarter Windows itself would use.
            placement::DragSnap m_dragSnap;
    };
}
