#include "gui.h"

namespace gui {
    const char* getUnitString(int unitCode) {
        switch (unitCode) {
            case 0: return "mm";
            case 1: return "cm";
            case 2: return "in";
            case 3: return "m";
            default: return "unknown";
        }
    }

    const char* getRayAimingTypeString(int rayAimingType) {
        switch (rayAimingType) {
            case 0: return "Off";
            case 1: return "Paraxial";
            case 2: return "Real";
            default: return "Unknown";
        }
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
        return glfwWindow ? glfwWindowShouldClose(glfwWindow) : true;
    }
}