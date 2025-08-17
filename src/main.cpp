#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "logger/logger.h"
#include "dde/dde_zemax_client.h"
#include "gui/gui.h"

Logger logger;

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    if (iMsg >= WM_DDE_FIRST && iMsg <= WM_DDE_LAST) {
        ZemaxDDE::ZemaxDDEClient* client = (ZemaxDDE::ZemaxDDEClient*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (client) {
            return client->handleDDEMessages(iMsg, wParam, lParam);    
        } 
#ifdef DEBUG_LOG
        else {
            logger.addLog("WndProc: No ZemaxDDEClient instance associated with hwnd = " + std::to_string((uintptr_t)hwnd));
        }
#endif
    }
    return DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
#ifdef DEBUG_LOG
        logger.addLog("Failed to initialize GLFW.");
#endif
        return -1;
    }

#ifdef DEBUG_LOG
    logger.addLog("GLFW initialized successfully.");
#endif

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    GLFWwindow* glfwWindow = glfwCreateWindow(800, 600, "ZemaxDDEClient", NULL, NULL);

    if (!glfwWindow) {
#ifdef DEBUG_LOG
        logger.addLog("Failed to create GLFW window.");
#endif
        glfwTerminate(); 
        return -1; 
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSwapInterval(1);

#ifdef DEBUG_LOG
    logger.addLog("Application starting");
#endif

    // Register DDE message-only window class
    WNDCLASSEXW wndClass = { 
        sizeof(WNDCLASSEXW),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        NULL,
        NULL,
        L"ZEMAX_DDE_Client",
        NULL
    };

    if (!RegisterClassExW(&wndClass)) {
#ifdef DEBUG_LOG
        logger.addLog("Failed to register DDE window class.");
#endif    
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
        return -1;
    }

#ifdef DEBUG_LOG
    logger.addLog("DDE window class registered.");
#endif

    HWND hwndClient = NULL;
    const char* const DDE_ERROR_MSG_CREATE_WIN = "Failed to create DDE window"; 
    hwndClient = CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hwndClient) {
        MessageBoxA(NULL, DDE_ERROR_MSG_CREATE_WIN, "Error", MB_OK | MB_ICONERROR);
#ifdef DEBUG_LOG
        logger.addLog(DDE_ERROR_MSG_CREATE_WIN);
#endif
        glfwTerminate();
        return -1;
    }

#ifdef DEBUG_LOG
    logger.addLog("DDE window created with handle hwndClient = " + std::to_string((uintptr_t)hwndClient));
#endif

    // Create and associate ZemaxDDEClient with the window
    ZemaxDDE::ZemaxDDEClient* zemaxDDEClient = new ZemaxDDE::ZemaxDDEClient(hwndClient);
    SetWindowLongPtr(hwndClient, GWLP_USERDATA, (LONG_PTR)zemaxDDEClient);

    // Initialize GUI manager with existing DDE client
    gui::GuiManager gui(glfwWindow, hwndClient, zemaxDDEClient);
    gui.initialize();

    while (!gui.shouldClose()) {
        glfwPollEvents();
        gui.render();
        glfwSwapBuffers(glfwWindow);
    }

    if (hwndClient) {
        ZemaxDDE::ZemaxDDEClient* client = (ZemaxDDE::ZemaxDDEClient*)GetWindowLongPtr(hwndClient, GWLP_USERDATA);
        if (client) {
            client->terminateDDE();
            logger.addLog("DDE connection terminated 2");
            delete client;
            SetWindowLongPtr(hwndClient, GWLP_USERDATA, 0);
        }
        DestroyWindow(hwndClient);
    }

    glfwDestroyWindow(glfwWindow);
    glfwTerminate();

    return 0;
}
