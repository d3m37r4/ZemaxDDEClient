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
#include "gui/sag_analysis_service.h"
#include "gui/sag_analysis_controller.h"
#include "gui/graphics_backend.h"
#include "gui/menu_bar_controller.h"
#include "gui/sidebar_renderer.h"
#include "gui/content_router.h"
#include "dde/client.h"
#include "dde/dde_connection_manager.h"

class Logger;

namespace gui {
    // Free utility functions
    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    void HelpMarker(const char* desc);
    void renderPageHeader(GuiPage currentPage);

    class GuiManager {
        public:
            GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);
            ~GuiManager();

            void initialize();
            void render();
            void updateDpiStyle(float dpiScale);

            void renderNavbar();
            void renderSidebar();
            void renderDDEStatusFrame();
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
            GLFWwindow* m_glfwWindow;
            HWND m_hwndClient;
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;
            Logger& m_logger;
            std::unique_ptr<SagAnalysisService> m_sagService;
            std::unique_ptr<SagAnalysisController> m_sagController;
            std::unique_ptr<DdeConnectionManager> m_ddeConnectionManager;
            std::unique_ptr<MenuBarController> m_menuBarController;
            std::unique_ptr<SidebarRenderer> m_sidebarRenderer;
            std::unique_ptr<ContentRouter> m_contentRouter;
            GraphicsBackend m_graphics;

            GuiPage m_currentPage = GuiPage::OpticalSystemInfo;

            bool m_showUpdatesPopup{false};                       // Display flag for popup 'Check for Updates'
            bool m_showAboutPopup{false};                         // Display flag for popup 'Check for About'
    };
}
