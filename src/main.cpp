#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include <string>
#include <vector>
#include <ctime>
#include <stdexcept>
#include "dde_client.h"
#include "gui.h"

std::vector<std::string> debug_log;
void AddDebugLog(const char* message) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%y - %H:%M:%S", timeinfo);
    debug_log.push_back(std::string(timestamp) + ": " + message);
}

ImFont* AddSystemFont(float size_pixels) {
    ImGuiIO& io = ImGui::GetIO();
    char fontPath[MAX_PATH];
    GetWindowsDirectoryA(fontPath, MAX_PATH);
    strcat_s(fontPath, "\\Fonts\\segoeui.ttf");
    return io.Fonts->AddFontFromFileTTF(fontPath, size_pixels);
}

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
        AddDebugLog("Failed to create DDE window");
        MessageBoxA(NULL, "Failed to create DDE window", "Error", MB_OK | MB_ICONERROR);
        glfwTerminate();
        return -1;
    }
    AddDebugLog((std::string("DDE window created with handle: ") + std::to_string((uintptr_t)hwndDDE)).c_str());

    GuiManager gui(window, hwndDDE); // Передаем hwndDDE
    gui.initialize();
    gui.setDebugLog(debug_log); // Устанавливаем debug_log для GUI
    ZemaxDDE::setDebugLog(debug_log); // Устанавливаем debug_log для DDE

    AddSystemFont(17.0f);

    while (!gui.shouldClose()) {
        glfwPollEvents(); // Обработка событий перед рендерингом
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
