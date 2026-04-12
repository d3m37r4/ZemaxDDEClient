#include <fstream>

#include "logger/logger.h"

#include "app/app.h"

namespace App {
    AppContext* initialize() {
        auto ctx = new AppContext();

        // Enable DPI awareness for proper scaling on high-DPI displays
        SetProcessDPIAware();

        // Initialize GLFW
        if (!glfwInit()) {
            logger.addLog("[APP] Failed to initialize GLFW");
            delete ctx;
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
            delete ctx;
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
            delete ctx;
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
            delete ctx;
            return nullptr;
        }

        logger.addLog(std::format("[APP] DDE window created: hwndClient = {}", (uintptr_t)ctx->hwndClient));

        // Create and associate ZemaxDDEClient with the window
        ctx->ddeClient = new ZemaxDDE::ZemaxDDEClient(ctx->hwndClient);
        SetWindowLongPtr(ctx->hwndClient, GWLP_USERDATA, (LONG_PTR)ctx->ddeClient);

        // Register callback to send requests after DDE connection
        ctx->ddeClient->setOnDDEConnectedCallback([](ZemaxDDE::ZemaxDDEClient* client) {
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

        // Initialize GUI manager with existing DDE client
        ctx->gui = new gui::GuiManager(ctx->glfwWindow, ctx->hwndClient, ctx->ddeClient);
        ctx->gui->initialize();

        // Set up DPI change callback for dynamic scaling
        glfwSetWindowContentScaleCallback(ctx->glfwWindow, [](GLFWwindow* window, float xScale, float yScale) {
            AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
            if (!ctx) return;

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
        glfwSetWindowUserPointer(ctx->glfwWindow, ctx);

        return ctx;
    }

    void shutdown(AppContext* ctx) {
        if (!ctx) return;

        // Shutdown GUI
        if (ctx->gui) {
            delete ctx->gui;
            ctx->gui = nullptr;
        }

        // Shutdown DDE
        if (ctx->hwndClient) {
            if (ctx->ddeClient) {
                ctx->ddeClient->terminateDDE();
                delete ctx->ddeClient;
                SetWindowLongPtr(ctx->hwndClient, GWLP_USERDATA, 0);
            }
            DestroyWindow(ctx->hwndClient);
            ctx->hwndClient = nullptr;
        }

        // Shutdown GLFW
        if (ctx->glfwWindow) {
            glfwDestroyWindow(ctx->glfwWindow);
            ctx->glfwWindow = nullptr;
        }

        glfwTerminate();

        delete ctx;
    }
}
