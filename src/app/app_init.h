#pragma once

#include <GLFW/glfw3.h>
#include <windows.h>
#include "dde/dde_zemax_client.h"
#include "gui/gui.h"

struct AppContext {
    GLFWwindow* glfwWindow = nullptr;
    HWND hwndClient = nullptr;
    ZemaxDDE::ZemaxDDEClient* ddeClient = nullptr;
    gui::GuiManager* gui = nullptr;
};

extern "C" LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Initialize entire application
 * @return Pointer to AppContext on success, nullptr on error
 */
AppContext* initializeApplication();

/**
 * @brief Frees up all resources
 * @param ctx Application context
 */
void shutdownApplication(AppContext* ctx);
