#pragma once

#include <stdexcept>
#include <string>

#include <windows.h>
#include <nfd.h>
#include <GLFW/glfw3.h>

#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"

#include "application.h"
#include "version.h"

#include "dde/dde_zemax_client.h"

#include "gui/components/gui_content.h"
#include "gui/components/gui_dde_status.h"
#include "gui/components/gui_debug_log.h"
#include "gui/components/gui_menu_bar.h"
#include "gui/components/gui_popups.h"
#include "gui/components/gui_sidebar.h"
#include "gui/content_pages/gui_page_local_surface_errors.h"
#include "gui/content_pages/gui_page_optical_system_info.h"

namespace gui {
    const char* getUnitString(int unitCode);
    const char* getRayAimingTypeString(int rayAimingType);

    class GuiManager {
        public: 
            GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient);
            ~GuiManager();

            void initialize();
            void render();

            void renderDebugLogFrame();
            void setPopupPosition();
            void renderAboutPopup();
            void renderUpdatesPopup();
            void renderDDEStatusFrame();
            void renderSidebar();
            void renderMenuBar();
            void renderContent();

            void renderPageOpticalSystemInfo();
            void renderPageLocalSurfaceErrors();

            bool shouldClose() const { return glfwWindow ? glfwWindowShouldClose(glfwWindow) : true; }
            bool isDdeInitialized() const { return zemaxDDEClient != nullptr && zemaxDDEClient->isConnected(); }

        private: 
            GLFWwindow* glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* zemaxDDEClient;           // Pointer to a DDE client instance

            int selectedMenuItem{0};
            bool show_updates_popup{false};                     // Display flag for popup 'Check for Updates'
            bool show_about_popup{false};                       // Display flag for popup 'Check for About'
    };
}
