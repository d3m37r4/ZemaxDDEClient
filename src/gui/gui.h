#pragma once

#include <filesystem>
#include <vector>

#include <windows.h>
#include <GLFW/glfw3.h>

#include <nfd.h>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "lib/implot/implot.h"

#include "version.h"
#include "dde/client.h"

#include "gui/constants.h"
#include "gui/content_pages/page_surface_sag_analysis.h"

namespace gui {
    enum class GuiPage {
        OpticalSystemInfo,      // = 0
        SurfaceSagAnalysis      // = 1
    };

    struct GuiPageInfo {
        GuiPage id;
        const char* title;
    };

    constexpr GuiPageInfo GUI_PAGES[] = {
        {GuiPage::OpticalSystemInfo, "Optical System Information"},
        {GuiPage::SurfaceSagAnalysis, "Surface Sag Cross Section Analysis"}
    };

    constexpr size_t GUI_PAGES_COUNT = sizeof(GUI_PAGES) / sizeof(GUI_PAGES[0]);

    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    
    void HelpMarker(const char* desc);
    void renderPageHeader(GuiPage currentPage);

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface);
    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

    class GuiManager {
        public: 
            GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient);
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
