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

    bool GuiManager::shouldClose() const {
        return glfwWindow ? glfwWindowShouldClose(glfwWindow) : true;
    }
}