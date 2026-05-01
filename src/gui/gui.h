#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <windows.h>
#include <GLFW/glfw3.h>

#include "gui/forwards.h"
#include "gui/constants.h"
#include "gui/utils.h"
#include "gui/sag_analysis_service.h"
#include "gui/sidebar_renderer.h"
#include "gui/content_router.h"
#include "gui/menu_bar_controller.h"
#include "gui/sag_analysis_controller.h"
#include "gui/graphics_backend.h"
#include "gui/debug_log_viewer.h"
#include "gui/app_info_dialog.h"
#include "dde/client.h"
#include "dde/dde_connection_manager.h"

class Logger;

namespace gui {
    // Free utility functions (declared in utils.h, included above)

    class GuiManager {
        public:
            GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
            ~GuiManager();

            void initialize(float dpiScale = 1.0f);
            void render();
            void updateDpiStyle(float dpiScale);

            // Delegated to components
            void renderSidebar();
            void renderContent();
            void renderDebugLog();
            void setPopupPosition();
            void renderAboutPopup();
            void renderUpdatesPopup();

            void renderPageOpticalSystemInfo();
            void renderPageSurfaceSagAnalysis();

            [[nodiscard]] bool shouldClose() const noexcept { return m_glfwWindow ? glfwWindowShouldClose(m_glfwWindow) : true; }
            [[nodiscard]] bool isDdeInitialized() const noexcept { return m_zemaxDDEClient != nullptr && m_zemaxDDEClient->isConnected(); }

        private:
            GLFWwindow* m_glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND m_hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;           // Pointer to a DDE client instance
            Logger& m_logger;                                     // Logger instance (dependency injection)

            // Components (New approach)
            GraphicsBackend m_graphics;
            std::unique_ptr<SagAnalysisService> m_sagService;     // Sag analysis service (owned by GuiManager)
            std::unique_ptr<SagAnalysisController> m_sagController;
            std::unique_ptr<DdeConnectionManager> m_ddeConnectionManager;
            std::unique_ptr<MenuBarController> m_menuBarController;
            std::unique_ptr<SidebarRenderer> m_sidebarRenderer;
            std::unique_ptr<ContentRouter> m_contentRouter;
            std::unique_ptr<DebugLogViewer> m_debugLogViewer;
            std::unique_ptr<AppInfoDialog> m_appInfoDialog;

            GuiPage m_currentPage = GuiPage::OpticalSystemInfo;

            // State
            bool m_showUpdatesPopup{false};                       // Display flag for popup 'Check for Updates'
            bool m_showAboutPopup{false};                         // Display flag for popup 'Check for About'
    };
}
