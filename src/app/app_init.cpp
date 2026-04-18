#include <fstream>

#include <nfd.h>

#include "logger/logger.h"
#include "app/app.h"

namespace {
    extern "C" LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
        if (iMsg >= WM_DDE_FIRST && iMsg <= WM_DDE_LAST) {
            auto* client = reinterpret_cast<ZemaxDDE::ZemaxDDEClient*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (client) {
                return client->handleDDEMessages(iMsg, wParam, lParam);
            }
        }
        return DefWindowProcW(hwnd, iMsg, wParam, lParam);
    }
}

namespace App {
    std::unique_ptr<AppContext> initialize(Logger& logger) {
        auto ctx = std::make_unique<AppContext>();

        ctx->pLogger = &logger;

        // Enable DPI awareness for proper scaling on high-DPI displays
        SetProcessDPIAware();

        // Initialize GLFW
        if (!glfwInit()) {
            logger.addLog("[APP] Failed to initialize GLFW");
            return nullptr;
        }

        logger.addLog("[APP] GLFW initialized");

        // Get initial DPI scale factor
        HDC hdc = GetDC(NULL);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        ctx->dpiScale = static_cast<float>(dpiX) / 96.0f;
        logger.addLog(std::format("[APP] Initial DPI scale factor: {:.2f} ({}x{})", ctx->dpiScale, dpiX, dpiY));

        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        // Scale window size based on DPI
        int baseWidth = 800;
        int baseHeight = 600;
        int scaledWidth = static_cast<int>(baseWidth * ctx->dpiScale);
        int scaledHeight = static_cast<int>(baseHeight * ctx->dpiScale);

        ctx->glfwWindow = glfwCreateWindow(scaledWidth, scaledHeight, APP_TITLE, NULL, NULL);

        if (!ctx->glfwWindow) {
            logger.addLog("[APP] Failed to create GLFW window");
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(ctx->glfwWindow);
        glfwSwapInterval(1);

        logger.addLog("[APP] Application starting");

        // Register DDE message-only window class
        WNDCLASSEXW wndClass = {};
        wndClass.cbSize = sizeof(WNDCLASSEXW);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = WndProc;
        wndClass.hInstance = GetModuleHandle(NULL);
        wndClass.lpszClassName = L"ZEMAX_DDE_Client";

        if (!RegisterClassExW(&wndClass)) {
            logger.addLog("[APP] Failed to register DDE window class");
            glfwDestroyWindow(ctx->glfwWindow);
            glfwTerminate();
            return nullptr;
        }

        logger.addLog("[APP] DDE window class registered");

        // Create and associate ZemaxDDEClient with the window
        ctx->hwndClient = CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

        if (!ctx->hwndClient) {
            MessageBoxA(NULL, "Failed to create DDE communication window.\n"
                            "The application will now exit.\n\n"
                            "Ensure no other instance is running and try again.", "Error", MB_OK | MB_ICONERROR);
            logger.addLog("[APP] Failed to create DDE window (CreateWindowExW returned NULL)");
            glfwTerminate();
            return nullptr;
        }

        logger.addLog(std::format("[APP] DDE window created: hwndClient = {}", reinterpret_cast<uintptr_t>(ctx->hwndClient)));

        // Create and associate ZemaxDDEClient with the window
        ctx->ddeClient = std::make_unique<ZemaxDDE::ZemaxDDEClient>(ctx->hwndClient, logger);
        SetWindowLongPtr(ctx->hwndClient, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx->ddeClient.get()));

        // Register callback to send requests after DDE connection
        ctx->ddeClient->setOnDDEConnectedCallback([&logger](ZemaxDDE::ZemaxDDEClient* client) {
            try {
                client->getLensName();
                client->getFileName();
                client->getSystemData();
                client->getFieldData();

                int numFields = client->getOpticalSystemData().numFields;
                if (numFields < 0) {
                    logger.addLog(std::format("[DDE] Invalid numFields value: {}. Skipping field requests.", numFields));
                    return;
                }

                for (int i = ZemaxDDE::MIN_FIELDS; i <= numFields; ++i) {
                    client->getFieldByIndex(i);
                }

                client->getWaveData();

                int numWaves = client->getOpticalSystemData().numWaves;
                if (numWaves < 0) {
                    logger.addLog(std::format("[DDE] Invalid numWaves value: {}. Skipping field requests.", numWaves));
                    return;
                }

                for (int i = ZemaxDDE::MIN_WAVES; i <= numWaves; ++i) {
                    client->getWaveByIndex(i);
                }
            } catch (const std::exception& e) {
                logger.addLog(std::format("[DDE] Error requesting initial system data: {}", e.what()));
            }
        });

        ctx->pLogger = &logger;

        // Initialize GUI manager with existing DDE client
        ctx->gui = std::make_unique<gui::GuiManager>(ctx->glfwWindow, ctx->hwndClient, ctx->ddeClient.get(), logger);
        ctx->gui->initialize();

        // Set up DPI change callback for dynamic scaling
        glfwSetWindowContentScaleCallback(ctx->glfwWindow, [](GLFWwindow* window, float xScale, float yScale) {
            AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
            if (!ctx || !ctx->pLogger) return;

            Logger& logger = *ctx->pLogger;
            float newDpiScale = (xScale + yScale) / 2.0f;
            logger.addLog(std::format("[APP] DPI scale changed to: {:.2f}", newDpiScale));

            // Update window size
            int baseWidth = 800;
            int baseHeight = 600;
            int scaledWidth = static_cast<int>(baseWidth * newDpiScale);
            int scaledHeight = static_cast<int>(baseHeight * newDpiScale);
            glfwSetWindowSize(window, scaledWidth, scaledHeight);

            // Update ImGui style
            ctx->gui->updateDpiStyle(newDpiScale);
            ctx->dpiScale = newDpiScale;
        });

        // Store context pointer for callback access
        glfwSetWindowUserPointer(ctx->glfwWindow, ctx.get());

        return ctx;
    }

    void shutdown(AppContext& ctx) {
        // 1. Shutdown GUI first (depends on DDE client and GLFW)
        ctx.gui.reset();

        // 2. Shutdown DDE (depends on GLFW window messages)
        if (ctx.hwndClient && ctx.ddeClient) {
            ctx.ddeClient->terminateDDE();
            SetWindowLongPtr(ctx.hwndClient, GWLP_USERDATA, 0);
            ctx.ddeClient.reset();
        }

        // 3. Destroy DDE message window
        if (ctx.hwndClient) {
            DestroyWindow(ctx.hwndClient);
            ctx.hwndClient = nullptr;
        }

        // 4. Shutdown GLFW
        if (ctx.glfwWindow) {
            glfwDestroyWindow(ctx.glfwWindow);
            ctx.glfwWindow = nullptr;
        }

        glfwTerminate();
        // 5. Context destroyed by unique_ptr
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
