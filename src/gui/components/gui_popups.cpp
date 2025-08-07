#include "lib/imgui/imgui.h"
#include "gui/components/gui_popups.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::setPopupPosition() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    void GuiManager::renderUpdatesPopup() {
        if (show_updates_popup) { 
            ImGui::OpenPopup("Check for Updates"); 
            show_updates_popup = false; 
        }

        if (ImGui::BeginPopupModal("Check for Updates", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Your software is up to date!");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    void GuiManager::renderAboutPopup() {
        if (show_about_popup) { 
            ImGui::OpenPopup("About"); 
            show_about_popup = false; 
        }

        if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("ZemaxDDEClient\nVersion 1.0\n\n(c) 2023 Your Company");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}
