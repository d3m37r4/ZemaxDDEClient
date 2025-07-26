#ifndef GUI_H
#define GUI_H

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include "logger/logger.h"

class GuiManager {
public:
    GuiManager(GLFWwindow* window, HWND hwndDDE = NULL);
    ~GuiManager();

    void initialize();
    void render();
    void setDDEStatus(bool initialized);
    void setSelectedMenuItem(int item);
    void setSurfaceNumber(int num);
    void setRadius(float r);
    void setErrorMsg(const char* msg);
    bool shouldClose() const;

private:
    GLFWwindow* window;
    HWND hwndDDE;
    Logger logger;
    bool dde_initialized;
    int selectedMenuItem;
    int surface_number;
    float radius;
    char errorMsg[256];
    bool show_about_popup;
    bool show_features_popup;
    bool show_updates_popup;
};

#endif
