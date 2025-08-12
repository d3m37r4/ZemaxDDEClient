#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "logger/logger.h"
#include "dde/dde_zemax_handler.h"
#include "gui/gui.h"

Logger logger;

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = ZemaxDDE::handleDDEMessages(hwnd, iMsg, wParam, lParam);
    if (result != 0) return result;
    return DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "ZemaxDDEClient", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    HWND hwndDDE = NULL;
#ifdef DEBUG_LOG
    logger.addLog("Application starting");
#endif
    WNDCLASSEXW wndclass = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0,
                             GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ZEMAX_DDE_Client", NULL };
#ifdef DEBUG_LOG
    logger.addLog("Registering window class");
    if (!RegisterClassExW(&wndclass)) {
        logger.addLog("Failed to register window class");
        return -1;
    }
    logger.addLog("Creating DDE window");
#else
    RegisterClassExW(&wndclass);
#endif

    hwndDDE = CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hwndDDE) {
        MessageBoxA(NULL, "Failed to create DDE window", "Error", MB_OK | MB_ICONERROR);
    #ifdef DEBUG_LOG
        logger.addLog("Failed to create DDE window");
    #endif
        glfwTerminate();
        return -1;
    }

#ifdef DEBUG_LOG
    logger.addLog("DDE window created with handle (hwndDDE): " + std::to_string((uintptr_t)hwndDDE));
#endif

    gui::GuiManager gui(window, hwndDDE);
    gui.initialize();

    while (!gui.shouldClose()) {
        glfwPollEvents();
        gui.render();
        glfwSwapBuffers(window);
    }

    if (hwndDDE) {
        ZemaxDDE::terminateDDE();
        DestroyWindow(hwndDDE);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
