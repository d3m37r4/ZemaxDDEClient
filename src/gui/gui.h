#pragma once

#include <memory>

#include <windows.h>
#include <GLFW/glfw3.h>

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/dockable_windows_manager.h"
#include "gui/utils.h"
#include "gui/surface_profile_service.h"
#include "gui/surface_irregularity_map_service.h"
#include "gui/ui_operation_monitor.h"
#include "windows_dockable/dde_status.h"
#include "gui/menu_bar_controller.h"
#include "gui/graphics_backend.h"
#include "windows_dockable/logs.h"
#include "gui/popups/about_dialog.h"
#include "gui/popups/shortcuts_help_dialog.h"
#include "gui/popups/connection_lost_dialog.h"
#include "gui/popups/preferences_dialog.h"
#include "gui/popups/update_checker.h"
#include "gui/settings_manager.h"
#include "dde/client.h"
#include "dde/dde_connection_manager.h"

class DockableWindowsManager;

class Logger;

#ifdef ZEMAX_ENABLE_TEST_ENGINE
class TestEngine;
#endif

namespace gui {
    class GuiManager {
        public:
            GuiManager(GLFWwindow* glfwWindow, DDEConnectionManager* ddeConnectionManager, Logger& logger);
            ~GuiManager();

            void initialize(bool isLightTheme, float dpiScale = 1.0f);
            void render();
            void updateDpiStyle(float dpiScale);

            /// Polls global hotkeys (see gui/shortcuts.h). Called once per frame
            /// from the main loop; safe to call before render().
            void handleShortcuts();
            /// True if any modal dialog is currently open (used as a hotkey guard).
            [[nodiscard]] bool isAnyModalOpen() const noexcept;
            /// Connect/disconnect split (replaces the old toggle):
            /// Opens the Connect popup if a slot is free (Ctrl+Shift+C).
            void openDdeConnect();
            /// Disconnects the active slot; no-op while tasks run (Ctrl+Shift+D).
            void disconnectDdeSlot();
            /// Opens the Check for Updates dialog (Ctrl+U).
            void openUpdates();
            /// Opens the keyboard shortcuts help dialog (F1).
            void openShortcutsHelp();
            /// Switches to the next connected DDE target (F6). No-op with <2 targets.
            void cycleDdeTarget();
            /// Selects a DDE target slot directly (Alt+1/Alt+2). No-op if empty.
            void selectDdeTarget(int slot);
            /// Toggles a dockable panel (View menu, Ctrl+1..5). No-op without a window manager.
            void toggleDockableWindow(WindowID id);
            /// Closes the topmost open dialog, innermost layer first (Esc).
            /// Returns true if anything was closed.
            bool closeTopmostPopup();

            void renderAboutPopup();
            void renderUpdatesPopup();
            void renderShortcutsHelpDialog();
            void renderPreferencesDialog();
            void renderConnectionLostPopup();

            SettingsManager& getSettingsManager() noexcept { return *m_settingsManager; }
            UpdateChecker* getUpdateChecker() const noexcept { return m_updateChecker.get(); }

            MenuBarController* getMenuBarController() { return m_menuBarController.get(); }
            void setWindowManager(DockableWindowsManager* wndMgr) { m_pWndMgr = wndMgr; }
            DockableWindowsManager* getWindowManager() const { return m_pWndMgr; }
            Logger& getLogger() const { return m_logger; }
            DDEStatus* getDDEStatusRenderer() const { return m_ddeStatusRenderer.get(); }

#ifdef ZEMAX_ENABLE_TEST_ENGINE
            /// Attaches the Dear ImGui Test Engine so its UI is rendered inside
            /// the application frame. Only available in test-enabled builds.
            void setTestEngine(TestEngine* engine) { m_testEngine = engine; }
            TestEngine* getTestEngine() const { return m_testEngine; }
#endif

            template<typename Func>
            void renderToolbarWindow(const char* title, bool* open, Func content) {
                ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_Tab));
                if (ImGui::Begin(title, open, ImGuiWindowFlags_MenuBar)) {
                    ImGuiUtils::ClampWindowToViewport();
                    content();
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }

            void renderOpticalSystemInfo();
            void renderSurfaceProfileInspector();
            void renderSurfaceIrregularityMap();
            void renderLogs();

            /// Body of the Welcome window: accent icon, short hint and the
            /// primary Connect CTA (same popup as the menu/shortcut path).
            /// The window itself (pos/size/visibility) is owned by
            /// DockableWindowsManager, which auto-hides it while connected.
            /// Window size of the Welcome card: measured content extent +
            /// card padding + title bar. Given to ImGui before the window
            /// appears so pivot centering lands exactly on the final size
            /// (AlwaysAutoResize would settle one frame later).
            [[nodiscard]] ImVec2 welcomeWindowSize() const;

            void renderWelcomeContent();

            /// Global status bar height, used to center the Welcome window
            /// over the dock area.
            [[nodiscard]] float computeStatusBarHeight() const { return m_uiOpMonitor.computeStatusBarHeight(); }

            [[nodiscard]] bool shouldClose() const noexcept { return m_glfwWindow ? glfwWindowShouldClose(m_glfwWindow) : true; }
            [[nodiscard]] bool isDDEInitialized() const noexcept { return m_zemaxDDEClient != nullptr && m_zemaxDDEClient->isConnected(); }

        private:
            /// Row metrics of the welcome card body at the current DPI.
            /// Shared by welcomeWindowSize() (window sizing) and
            /// renderWelcomeContent() (centering) so both stay in sync.
            struct WelcomeMetrics {
                ImVec2 icon{};   // drawing extent at the enlarged font
                ImVec2 title{};
                ImVec2 hint1{};
                ImVec2 hint2{};
                ImVec2 button{}; // filled button incl. frame padding
                float wrap = 0.0f; // hint wrap width, DPI-scaled
                float gap = 0.0f;  // row gap, DPI-scaled
            };
            void measureWelcome(WelcomeMetrics& m) const;

            // Non-owning. Lifetime guaranteed by AppContext (outlives GuiManager).
            GLFWwindow* m_glfwWindow;
            DDEConnectionManager* m_ddeConnectionManager;
            // Non-owning. Nullable — refreshed each frame in render().
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;
            Logger& m_logger;

            GraphicsBackend m_graphics;
            std::unique_ptr<SurfaceProfileService> m_profileService;
            std::unique_ptr<SurfaceIrregularityMapService> m_irregularityMapService;
            std::unique_ptr<MenuBarController> m_menuBarController;
            std::unique_ptr<DDEStatus> m_ddeStatusRenderer;
            std::unique_ptr<Logs> m_logsRenderer;
            std::unique_ptr<AboutDialog> m_aboutDialog;
            std::unique_ptr<ShortcutsHelpDialog> m_shortcutsHelpDialog;
            std::unique_ptr<ConnectionLostDialog> m_connectionLostDialog;
            std::unique_ptr<UpdateChecker> m_updateChecker;
            std::unique_ptr<SettingsManager>   m_settingsManager;
            std::unique_ptr<PreferencesDialog> m_preferencesDialog;
            // Non-owning. Optional — set via setWindowManager(), lifetime from main().
            DockableWindowsManager* m_pWndMgr{nullptr};

            UiOperationMonitor m_uiOpMonitor;
            unsigned int m_frameCount = 0;
            int m_pendingDisconnectIndex = -1;

#ifdef ZEMAX_ENABLE_TEST_ENGINE
            TestEngine* m_testEngine = nullptr;
#endif
    };
}
