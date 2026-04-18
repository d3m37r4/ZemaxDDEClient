#include "gui/gui.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void GuiManager::renderUpdatesPopup() {
        if (m_showUpdatesPopup) {
            ImGui::OpenPopup("Check for Updates");
            m_showUpdatesPopup = false;
        }

        if (ImGui::BeginPopupModal("Check for Updates", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Your software is up to date!");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}
