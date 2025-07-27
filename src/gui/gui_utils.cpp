#include "gui.h"

namespace gui {
    void GuiManager::setDDEStatus(bool initialized) {
        dde_initialized = initialized;
    }

    void GuiManager::setSelectedMenuItem(int item) {
        selectedMenuItem = item;
    }

    void GuiManager::setSurfaceNumber(int num) {
        surface_number = num;
    }

    void GuiManager::setRadius(float r) {
        radius = r;
    }

    void GuiManager::setErrorMsg(const char* msg) {
        if (msg) {
            strncpy_s(errorMsg, msg, sizeof(errorMsg) - 1);
            errorMsg[sizeof(errorMsg) - 1] = '\0';
        } else {
            errorMsg[0] = '\0';
        }
    }

    bool GuiManager::shouldClose() const {
        return window ? glfwWindowShouldClose(window) : true;
    }
}