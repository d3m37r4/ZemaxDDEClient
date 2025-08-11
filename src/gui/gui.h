#ifndef GUI_H
#define GUI_H

#include <GLFW/glfw3.h>
#include <windows.h>
#include "logger/logger.h"

class ImGuiIO;                                                  // Forward declaration for ImGui usage

namespace gui {
    class GuiManager {
        public: 
            GuiManager(GLFWwindow* window, HWND hwndDDE = nullptr);
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

            void setDDEStatus(bool initialized);
            void setSelectedMenuItem(int item);
            void setSurfaceNumber(int num);
            void setRadius(float r);
            void setErrorMsg(const char* msg);
            bool shouldClose() const;

        private: 
            static constexpr int ERROR_MSG_SIZE = 256;          // Size of the error message buffer

            GLFWwindow* window;                                 // Pointer to the GLFW window handle
            HWND hwndDDE;                                       // Handle to the DDE window
            Logger logger;                                      // Logger instance for tracking events
            
            char errorMsg[ERROR_MSG_SIZE]{0};                   // Buffer for error messages
            bool dde_initialized{false};                        // Flag indicating DDE connection status
            int selectedMenuItem{0};
            int surface_number{1};
            float radius{0.0f};
            bool show_updates_popup{false};                     // Display flag for popup 'Check for Updates'
            bool show_about_popup{false};                       // Display flag for popup 'Check for About'
    };
}
#endif // GUI_H
