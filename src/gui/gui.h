#pragma once

#include <GLFW/glfw3.h>
#include "dde/dde_zemax_client.h"

class ImGuiIO;                                                  // Forward declaration for ImGui usage

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

            bool shouldClose() const;

        private: 
            // static constexpr int ERROR_MSG_SIZE = 256;          // Size of the error message buffer

            GLFWwindow* glfwWindow;                             // Pointer to handle of GLFW graphics window used for rendering interface
            HWND hwndClient;                                    // DDE client window handle
            ZemaxDDE::ZemaxDDEClient* zemaxDDEClient;           // Pointer to a DDE client instance
            
            // char errorMsg[ERROR_MSG_SIZE]{0};                   // Buffer for error messages
            bool dde_initialized{false};                        // Flag indicating DDE connection status
            int selectedMenuItem{0};
            // int surface_number{1};
            // float radius{0.0f};
            bool show_updates_popup{false};                     // Display flag for popup 'Check for Updates'
            bool show_about_popup{false};                       // Display flag for popup 'Check for About'
    };
}
