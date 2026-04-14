#pragma once

#include <memory>
#include <GLFW/glfw3.h>
#include <windows.h>

#include "dde/client.h"
#include "gui/gui.h"
#include "version.h"

class Logger;
namespace gui { class SagAnalysisService; }

#define APP_NAME "ZemaxDDEClient"
#define APP_TITLE APP_NAME " " APP_FULL_VERSION

struct AppContext {
    GLFWwindow* glfwWindow = nullptr;
    HWND hwndClient = nullptr;
    std::unique_ptr<ZemaxDDE::ZemaxDDEClient> ddeClient;
    std::unique_ptr<gui::GuiManager> gui;
    Logger* pLogger = nullptr;
    float dpiScale = 1.0f;
};

namespace App {
    /**
     * @brief Initialize entire application: GLFW, DDE window, DDE client, GUI.
     * @param logger Logger instance for logging (dependency injection).
     * @return Unique pointer to AppContext on success, nullptr on error.
     */
    std::unique_ptr<AppContext> initialize(Logger& logger);

    /**
     * @brief Shut down and clean up all resources.
     * @param ctx Application context (must not be used after this call).
     */
    void shutdown(AppContext& ctx);

    /**
     * @brief Open a .zmx file in Zemax using native file dialog and ShellExecute.
     */
    void openZmxFileInZemax(Logger& logger);
}
