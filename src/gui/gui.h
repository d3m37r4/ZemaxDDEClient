#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <windows.h>
#include <GLFW/glfw3.h>

#include "gui/constants.h"
#include "gui/utils.h"
#include "gui/surface_profile_service.h"
#include "gui/surface_irregularity_map_service.h"
#include "gui/ui_operation_monitor.h"
#include "windows_dockable/dde_status.h"
#include "gui/menu_bar_controller.h"
#include "gui/graphics_backend.h"
#include "windows_dockable/debug_log.h"
#include "gui/popups/about_dialog.h"
#include "gui/popups/preferences_dialog.h"
#include "gui/popups/update_checker.h"
#include "gui/settings_manager.h"
#include "dde/client.h"
#include "dde/dde_connection_manager.h"

class DockableWindowsManager;

class Logger;

namespace gui {
    class GuiManager {
        public:
            GuiManager(GLFWwindow* glfwWindow, DDEConnectionManager* ddeConnectionManager, Logger& logger);
            ~GuiManager();

            void initialize(bool isLightTheme, float dpiScale = 1.0f);
            void render();
            void updateDpiStyle(float dpiScale);

            void renderAboutPopup();
            void renderUpdatesPopup();
            void renderPreferencesDialog();

            MenuBarController* getMenuBarController() { return m_menuBarController.get(); }
            void setWindowManager(DockableWindowsManager* wndMgr) { m_pWndMgr = wndMgr; }
            DockableWindowsManager* getWindowManager() const { return m_pWndMgr; }
            DDEConnectionManager* getDDEConnectionManager() const { return m_ddeConnectionManager; }
            ZemaxDDE::ZemaxDDEClient* getDDEClient() const { return m_zemaxDDEClient; }
            Logger& getLogger() const { return m_logger; }
            DDEStatus* getDDEStatusRenderer() const { return m_ddeStatusRenderer.get(); }

            void renderOpticalSystemInfo();
            void renderSurfaceProfileInspector();
            void renderSurfaceIrregularityMap();
            void renderDebugLog();

            [[nodiscard]] bool shouldClose() const noexcept { return m_glfwWindow ? glfwWindowShouldClose(m_glfwWindow) : true; }
            [[nodiscard]] bool isDDEInitialized() const noexcept { return m_zemaxDDEClient != nullptr && m_zemaxDDEClient->isConnected(); }

        private:
            GLFWwindow* m_glfwWindow;
            DDEConnectionManager* m_ddeConnectionManager;
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;
            Logger& m_logger;

            GraphicsBackend m_graphics;
            std::unique_ptr<SurfaceProfileService> m_profileService;
            std::unique_ptr<SurfaceIrregularityMapService> m_irregularityMapService;
            std::unique_ptr<MenuBarController> m_menuBarController;
            std::unique_ptr<DDEStatus> m_ddeStatusRenderer;
            std::unique_ptr<DebugLog> m_debugLogRenderer;
            std::unique_ptr<AboutDialog> m_aboutDialog;
            std::unique_ptr<UpdateChecker> m_updateChecker;
            std::unique_ptr<SettingsManager>   m_settingsManager;
            std::unique_ptr<PreferencesDialog> m_preferencesDialog;
            DockableWindowsManager* m_pWndMgr{nullptr};

            UiOperationMonitor m_uiOpMonitor;

            // State
            bool m_showUpdatesPopup{false};
            bool m_showAboutPopup{false};
    };
}
