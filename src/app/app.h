#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include <windows.h>

#include "dde/client.h"
#include "gui/gui.h"
#include "version.h"

#define APP_NAME "ZemaxDDEClient"
#define APP_TITLE APP_NAME " " APP_FULL_VERSION

struct AppContext {
    GLFWwindow* glfwWindow = nullptr;
    HWND hwndClient = nullptr;
    std::unique_ptr<ZemaxDDE::ZemaxDDEClient> ddeClient;
    std::unique_ptr<gui::GuiManager> gui;
    float dpiScale = 1.0f;
};

namespace App {
    /**
     * @brief Initialize entire application: GLFW, DDE window, DDE client, GUI.
     * @return Pointer to AppContext on success, nullptr on error.
     */
    AppContext* initialize();

    /**
     * @brief Shut down and clean up all resources.
     * @param ctx Application context (must not be used after this call).
     */
    void shutdown(AppContext* ctx);

    /**
     * @brief Open a .zmx file in Zemax using native file dialog and ShellExecute.
     */
    void openZmxFileInZemax();
}

// Windows message handler (C linkage, global scope)
extern "C" LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
