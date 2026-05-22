#include <nfd.h>

#include "logger/logger.h"
#include "app/app.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace App {
    // Base window dimensions and system DPI constant
    constexpr int BASE_WIDTH = 800;
    constexpr int BASE_HEIGHT = 600;

    std::unique_ptr<AppContext> initialize(Logger& logger) {
        auto ctx = std::make_unique<AppContext>();

        ctx->pLogger = &logger;

        // Enable DPI awareness for proper scaling on high-DPI displays
        // Dynamically load SetProcessDpiAwarenessContext to avoid MinGW header issues
        HMODULE user32 = LoadLibraryW(L"user32.dll");
        if (user32) {
            using SetProcessDpiAwarenessContextFunc = BOOL (WINAPI*)(void*);
            auto fp = GetProcAddress(user32, "SetProcessDpiAwarenessContext");
            auto* func = reinterpret_cast<SetProcessDpiAwarenessContextFunc>(reinterpret_cast<void*>(fp));
            if (func) {
                // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (DPI_AWARENESS_CONTEXT)-4
                func((void*)-4);
                logger.addLog("[APP] DPI Awareness set to Per-Monitor V2 (runtime)");
            } else {
                SetProcessDPIAware();
                logger.addLog("[APP] DPI Awareness set to legacy mode (fallback)");
            }
            FreeLibrary(user32);
        } else {
            SetProcessDPIAware();
            logger.addLog("[APP] DPI Awareness set to legacy mode (user32 load failed)");
        }

        // Initialize GLFW
        if (!glfwInit()) {
            logger.addLog("[APP] Failed to initialize GLFW");
            return nullptr;
        }

        logger.addLog("[APP] GLFW initialized");

        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        ctx->glfwWindow = glfwCreateWindow(BASE_WIDTH, BASE_HEIGHT, APP_TITLE, NULL, NULL);
        
        if (!ctx->glfwWindow) {
            logger.addLog("[APP] Failed to create GLFW window");
            glfwTerminate();
            return nullptr;
        }

        float xScale = 0, yScale = 0;
        glfwGetWindowContentScale(ctx->glfwWindow, &xScale, &yScale);
        ctx->dpiScale = xScale;
        logger.addLog(std::format("[APP] Initial DPI scale factor: {:.2f}", ctx->dpiScale));

        glfwMakeContextCurrent(ctx->glfwWindow);
        glfwSwapInterval(1);

        // Apply system theme to title bar (Windows 10/11)
        // Define the attribute constant (MinGW headers may lack it)
        #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
        #define DWMWA_USE_IMMERSIVE_DARK_MODE 19
        #endif

        HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
        if (dwmApi) {
            using DwmSetWindowAttributeFunc = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            auto* dwmFunc = reinterpret_cast<DwmSetWindowAttributeFunc>(
                reinterpret_cast<void*>(GetProcAddress(dwmApi, "DwmSetWindowAttribute")));
            
            if (dwmFunc) {
                // Detect system dark mode via registry (reliable for Windows 10/11)
                bool isDarkMode = false;
                HKEY hKey = nullptr;
                LONG regResult = RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                    0, KEY_READ, &hKey);
                if (regResult == ERROR_SUCCESS) {
                    DWORD lightTheme = 1;
                    DWORD dataSize = sizeof(DWORD);
                    if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                            (LPBYTE)&lightTheme, &dataSize) == ERROR_SUCCESS) {
                        // lightTheme == 0 means dark mode is active
                        isDarkMode = (lightTheme == 0);
                    }
                    RegCloseKey(hKey);
                }

                BOOL useDarkMode = isDarkMode;
                HWND hwnd = glfwGetWin32Window(ctx->glfwWindow);
                if (hwnd) {
                    dwmFunc(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
                }
                
                logger.addLog(std::format("[APP] Title bar theme applied (dark mode: {})", isDarkMode));
            }
            FreeLibrary(dwmApi);
        }

        logger.addLog("[APP] Application starting");

        // Create DDE connection manager (manages multi-window connections)
        ctx->ddeConnectionManager = std::make_unique<DDEConnectionManager>(logger);

        // Initialize GUI manager with DDE connection manager
        ctx->gui = std::make_unique<gui::GuiManager>(ctx->glfwWindow, ctx->ddeConnectionManager.get(), logger);
        ctx->gui->initialize(ctx->dpiScale);

        // Store context pointer for callback access (must be before callback registration)
        glfwSetWindowUserPointer(ctx->glfwWindow, ctx.get());

        // Set up DPI change callback for dynamic scaling
        glfwSetWindowContentScaleCallback(ctx->glfwWindow, [](GLFWwindow* window, float xScale, float yScale) {
            AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
            if (!ctx || !ctx->pLogger) return;

            Logger& logger = *ctx->pLogger;
            float newDpiScale = (xScale + yScale) / 2.0f;

            // Update ImGui style
            ctx->gui->updateDpiStyle(newDpiScale);
            ctx->dpiScale = newDpiScale;

            logger.addLog(std::format("[APP] DPI scale changed to: {:.2f}", newDpiScale));
        });

        return ctx;
    }

    void shutdown(AppContext& ctx) {
        // GUI depends on DDE connection manager and GLFW
        ctx.gui.reset();

        // DDE connection manager cleanup (disconnects all clients, destroys windows)
        ctx.ddeConnectionManager.reset();

        if (ctx.glfwWindow) {
            glfwDestroyWindow(ctx.glfwWindow);
            ctx.glfwWindow = nullptr;
        }

        glfwTerminate();
    }

    void openZmxFileInZemax(Logger& logger) {
        nfdchar_t* outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog("zmx", nullptr, &outPath);

        if (result == NFD_OKAY) {
            struct NFDDeleter { void operator()(nfdchar_t* p) const { std::free(p); } };
            std::unique_ptr<nfdchar_t, NFDDeleter> pathGuard{outPath};

            #ifdef DEBUG_LOG
            logger.addLog(std::format("[APP] Selected file: {}", outPath));
            #endif

            int size = MultiByteToWideChar(CP_UTF8, 0, outPath, -1, nullptr, 0);
            std::wstring widePath(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, outPath, -1, widePath.data(), size);

            HINSTANCE ret = ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOW);
            if (reinterpret_cast<intptr_t>(ret) <= 32) {
                MessageBoxW(nullptr, L"Failed to open file. Is Zemax installed?", L"Error", MB_ICONERROR);
                logger.addLog(std::format("[APP] ShellExecute failed to open file: {}. (Error code: {})", outPath, static_cast<int>(reinterpret_cast<intptr_t>(ret))));
            } else {
                #ifdef DEBUG_LOG
                logger.addLog(std::format("[APP] Successfully sent file to ShellExecute: {}", outPath));
                #endif
            }
        } else if (result == NFD_CANCEL) {
            #ifdef DEBUG_LOG
            logger.addLog("[APP] File open dialog canceled by user");
            #endif
        } else {
            const char* error = NFD_GetError();
            logger.addLog(std::format("[APP] NFD Error: {}", error ? error : "Unknown error"));
        }
    }
}
