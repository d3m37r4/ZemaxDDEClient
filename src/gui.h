#ifndef GUI_H
#define GUI_H

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <windows.h>

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
    void setDebugLog(std::vector<std::string>& log);
    void AddDebugLog(const char* message); // Объявление функции

private:
    GLFWwindow* window;
    HWND hwndDDE;
    bool dde_initialized;
    int selectedMenuItem;
    int surface_number;
    float radius;
    char errorMsg[256];
    bool show_about_popup;
    bool show_features_popup;
    bool show_updates_popup;
    std::vector<std::string>* debug_log;
};

#endif
