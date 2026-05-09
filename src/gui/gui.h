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
#include "gui/sag_analysis_service.h"
#include "windows/dde_status_renderer.h"
#include "gui/menu_bar_controller.h"
#include "gui/graphics_backend.h"
#include "windows/debug_log_viewer.h"
#include "gui/app_info_dialog.h"
#include "dde/client.h"
#include "dde/dde_connection_manager.h"

class WindowManager;

class Logger;

namespace gui {
    class GuiManager {
        public:
            GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
            ~GuiManager();

            void initialize(float dpiScale = 1.0f);
            void render();
            void updateDpiStyle(float dpiScale);

            void renderDebugLog(bool* pOpen = nullptr);
            void setPopupPosition();
            void renderAboutPopup();
            void renderUpdatesPopup();

            MenuBarController* getMenuBarController() { return m_menuBarController.get(); }
            void setWindowManager(WindowManager* wndMgr) { m_pWndMgr = wndMgr; }
            WindowManager* getWindowManager() const { return m_pWndMgr; }
            void setShowDdeStatus(bool show) { m_showDdeStatus = show; }

            ZemaxDDE::ZemaxDDEClient* getDdeClient() const { return m_zemaxDDEClient; }
            Logger& getLogger() const { return m_logger; }
            DdeStatusRenderer* getDdeStatusRenderer() const { return m_ddeStatusRenderer.get(); }

            void renderOpticalSystemInfo();
            void renderSurfaceSagAnalysis();

            [[nodiscard]] bool shouldClose() const noexcept { return m_glfwWindow ? glfwWindowShouldClose(m_glfwWindow) : true; }
            [[nodiscard]] bool isDdeInitialized() const noexcept { return m_zemaxDDEClient != nullptr && m_zemaxDDEClient->isConnected(); }

        private:
            GLFWwindow* m_glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND m_hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;           // Pointer to a DDE client instance
            Logger& m_logger;                                     // Logger instance (dependency injection)

            GraphicsBackend m_graphics;
            std::unique_ptr<SagAnalysisService> m_sagService;
            std::unique_ptr<DdeConnectionManager> m_ddeConnectionManager;
            std::unique_ptr<MenuBarController> m_menuBarController;
            std::unique_ptr<DdeStatusRenderer> m_ddeStatusRenderer;
            std::unique_ptr<DebugLogViewer> m_debugLogViewer;
            std::unique_ptr<AppInfoDialog> m_appInfoDialog;
            WindowManager* m_pWndMgr{nullptr};

            // State
            bool m_showUpdatesPopup{false};
            bool m_showAboutPopup{false};
            bool m_showDdeStatus{true};
    };
}
