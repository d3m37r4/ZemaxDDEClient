#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "logger/logger.h"
#include "app_init.h"

AppContext* initializeApplication() {
    auto ctx = new AppContext();

    // Initialize GLFW
    if (!glfwInit()) {
        logger.addLog("Failed to initialize GLFW");
        delete ctx;
        return nullptr;
    }

    logger.addLog("GLFW initialized successfully.");

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    ctx->glfwWindow = glfwCreateWindow(800, 600, "ZemaxDDEClient", NULL, NULL);

    if (!ctx->glfwWindow) {
        logger.addLog("Failed to create GLFW window.");
        glfwTerminate();
        delete ctx;
        return nullptr;
    }

    glfwMakeContextCurrent(ctx->glfwWindow);
    glfwSwapInterval(1);

    logger.addLog("Application starting");

    // Register DDE message-only window class
    WNDCLASSEXW wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEXW);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.lpszClassName = L"ZEMAX_DDE_Client";

    if (!RegisterClassExW(&wndClass)) {
        logger.addLog("Failed to register DDE window class.");
        glfwDestroyWindow(ctx->glfwWindow);
        glfwTerminate();
        delete ctx;
        return nullptr;
    }

    logger.addLog("DDE window class registered.");

    // Create and associate ZemaxDDEClient with the window
    const char* const DDE_ERROR_MSG_CREATE_WIN = "Failed to create DDE window";
    ctx->hwndClient = CreateWindowExW(0, L"ZEMAX_DDE_Client", L"DDE Client", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

    if (!ctx->hwndClient) {
        MessageBoxA(NULL, DDE_ERROR_MSG_CREATE_WIN, "Error", MB_OK | MB_ICONERROR);
        logger.addLog(DDE_ERROR_MSG_CREATE_WIN);
        glfwTerminate();
        delete ctx;
        return nullptr;
    }

    logger.addLog("DDE window created with handle hwndClient = " + std::to_string((uintptr_t)ctx->hwndClient));

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
                logger.addLog("Invalid numFields value: " + std::to_string(numFields) + ". Skipping field requests.");
                return;
            }

            for (int i = ZemaxDDE::MIN_FIELDS; i <= numFields; ++i) {
                client->getFieldByIndex(i);
            }

            client->getWaveData();

            int numWaves = client->getOpticalSystemData().numWaves;
            if (numWaves < 0) {
                logger.addLog("Invalid numWaves value: " + std::to_string(numWaves) + ". Skipping wave requests.");
                return;
            }

            for (int i = ZemaxDDE::MIN_WAVES; i <= numWaves; ++i) {
                client->getWaveByIndex(i);
            }
        } catch (const std::exception& e) {
            logger.addLog("Error requesting initial system data: " + std::string(e.what()));
        }
    });

    // Initialize GUI manager with existing DDE client
    ctx->gui = new gui::GuiManager(ctx->glfwWindow, ctx->hwndClient, ctx->ddeClient);
    ctx->gui->initialize();

    return ctx;
}

void shutdownApplication(AppContext* ctx) {
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
