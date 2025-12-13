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
#include "dde/dde_zemax_client.h"

#include "gui/content_pages/gui_page_optical_system_info.h"
#include "gui/content_pages/gui_page_surface_sag_analysis.h"

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

            bool shouldClose() const { return glfwWindow ? glfwWindowShouldClose(glfwWindow) : true; }
            bool isDdeInitialized() const { return zemaxDDEClient != nullptr && zemaxDDEClient->isConnected(); }

        private: 
            GLFWwindow* glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* zemaxDDEClient;           // Pointer to a DDE client instance

            GuiPage currentPage = GuiPage::OpticalSystemInfo;
            SurfaceSagAnalysisPageState surfaceSagAnalysisPageState{};

            bool showTolerancedSagWindow{false};
            bool showNominalSagWindow{false};
            bool showComparisonWindow{false};
            bool showErrorWindow{false};
            
            bool show_updates_popup{false};                     // Display flag for popup 'Check for Updates'
            bool show_about_popup{false};                       // Display flag for popup 'Check for About'
    };
}
