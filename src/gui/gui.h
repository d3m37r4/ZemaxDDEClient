#pragma once

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

#include <windows.h>
#include <GLFW/glfw3.h>

#include "gui/forwards.h"
#include "gui/constants.h"
#include "gui/content_pages/page_surface_sag_analysis.h"
#include "dde/client.h"

class Logger;

namespace gui {
    // Free utility functions
    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    void HelpMarker(const char* desc);
    void renderPageHeader(GuiPage currentPage);

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface);
    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

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

            void renderSagCrossSectionWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag);
            void renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);
            void renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);

            void calculateSagCrossSection(int surface, int sampling, double angle = 0.0);
            void saveSagCrossSectionToFile(const ZemaxDDE::SurfaceData& surface);

            [[nodiscard]] bool shouldClose() const noexcept { return m_glfwWindow ? glfwWindowShouldClose(m_glfwWindow) : true; }
            [[nodiscard]] bool isDdeInitialized() const noexcept { return m_zemaxDDEClient != nullptr && m_zemaxDDEClient->isConnected(); }

        private:
            GLFWwindow* m_glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND m_hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* m_zemaxDDEClient;           // Pointer to a DDE client instance
            Logger& m_logger;                                     // Logger instance (dependency injection)

            GuiPage m_currentPage = GuiPage::OpticalSystemInfo;
            SurfaceSagAnalysisPageState m_surfaceSagAnalysisPageState{};

            bool m_showTolerancedSagWindow{false};
            bool m_showNominalSagWindow{false};
            bool m_showComparisonWindow{false};
            bool m_showErrorWindow{false};

            bool m_showUpdatesPopup{false};                       // Display flag for popup 'Check for Updates'
            bool m_showAboutPopup{false};                         // Display flag for popup 'Check for About'
    };
}
