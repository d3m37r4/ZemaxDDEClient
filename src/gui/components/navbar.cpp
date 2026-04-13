#include "gui/gui.h"
#include "gui/constants.h"

#include "app/app.h"

namespace gui {
    void GuiManager::renderNavbar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Open *.ZMX file in Zemax", "Ctrl+O")) App::openZmxFileInZemax();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) glfwSetWindowShouldClose(m_glfwWindow, true);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                // if (ImGui::MenuItem("Check for updates")) m_showUpdatesPopup = true;
                if (ImGui::MenuItem("About")) m_showAboutPopup = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}
