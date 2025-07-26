#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "logger/logger.h"
#include "dde_client.h"
#include "gui.h"

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
    bool dde_initialized = false;

    WNDCLASSEXW wndclass = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0,
                             GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ZEMAX_DDE_Client", NULL };
    RegisterClassExW(&wndclass);
    hwndDDE = CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hwndDDE) {
        MessageBoxA(NULL, "Failed to create DDE window", "Error", MB_OK | MB_ICONERROR);
        logger.addLog("Failed to create DDE window");
        glfwTerminate();
        
        return -1;
    }

    logger.addLog((std::string("DDE window created with handle: ") + std::to_string((uintptr_t)hwndDDE)).c_str());

    GuiManager gui(window, hwndDDE);
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
