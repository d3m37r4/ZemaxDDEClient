#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <math.h>
#include <algorithm>

#include <windows.h>
#include <nfd.h>
#include <GLFW/glfw3.h>

#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"

#include "lib/implot/implot.h"

#include "application.h"
#include "version.h"

#include "dde/dde_zemax_client.h"

#include "gui/components/gui_navbar.h"
#include "gui/components/gui_sidebar.h"
#include "gui/components/gui_dde_status.h"
#include "gui/components/gui_content.h"
#include "gui/components/gui_debug_log.h"
#include "gui/components/gui_popups.h"
#include "gui/content_pages/gui_page_local_surface_errors.h"
#include "gui/content_pages/gui_page_optical_system_info.h"

namespace gui {
    const char* getUnitString(int unitCode, bool full = false);
    const char* getRayAimingTypeString(int rayAimingType);
    void HelpMarker(const char* desc);

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
            void renderDebugLogFrame();
            void setPopupPosition();
            void renderAboutPopup();
            void renderUpdatesPopup();

            void renderPageOpticalSystemInfo();
            void renderPageLocalSurfaceErrors();

            void renderProfileWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag);
            void renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);
            void renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);

            void calculateSurfaceProfile(int surfaceNumber, int sampling, double angle = 0.0);
            void saveSagProfileToFile(const ZemaxDDE::SurfaceData& surface);

            bool shouldClose() const { return glfwWindow ? glfwWindowShouldClose(glfwWindow) : true; }
            bool isDdeInitialized() const { return zemaxDDEClient != nullptr && zemaxDDEClient->isConnected(); }

        private: 
            GLFWwindow* glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* zemaxDDEClient;           // Pointer to a DDE client instance

            LocalSurfaceErrorsPageState surfaceErrorsPageState{};
            int selectedMenuItem{0};

            bool showTolerancedProfileWindow{false};
            bool showNominalProfileWindow{false};
            bool showComparisonWindow{false};
            bool showErrorWindow{false};
            
            bool show_updates_popup{false};                     // Display flag for popup 'Check for Updates'
            bool show_about_popup{false};                       // Display flag for popup 'Check for About'
    };
}
