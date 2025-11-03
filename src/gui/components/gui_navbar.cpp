#include "gui/gui.h"

namespace gui {
    void GuiManager::renderNavbar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Open *.ZMX file in Zemax", "Ctrl+O")) Application::openZmxFileInZemax();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) glfwSetWindowShouldClose(glfwWindow, true);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                if (ImGui::MenuItem("Check for updates")) show_updates_popup = true;
                if (ImGui::MenuItem("About")) show_about_popup = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}
