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
#include "dde/client.h"

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

            void initialize(float dpiScale = 1.0f);
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
            GLFWwindow* m_glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND m_hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;           // Pointer to a DDE client instance
            Logger& m_logger;                                     // Logger instance (dependency injection)
            std::unique_ptr<SagAnalysisService> m_sagService;     // Sag analysis service (owned by GuiManager)

            GuiPage m_currentPage = GuiPage::OpticalSystemInfo;

            bool m_showUpdatesPopup{false};                       // Display flag for popup 'Check for Updates'
            bool m_showAboutPopup{false};                         // Display flag for popup 'Check for About'
    };
}
