#include "lib/imgui/imgui.h"
#include "gui/gui.h"
#include "gui/components/gui_menu_bar.h"

namespace gui {
    void GuiManager::renderMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                ImGui::MenuItem("Open *.ZMX file with Zemax");
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) glfwSetWindowShouldClose(window, true);
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
