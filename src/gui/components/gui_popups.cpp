#include "lib/imgui/imgui.h"
#include "gui/components/gui_popups.h"
#include "gui/gui.h"
#include "version.h"

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
            ImGui::Text("ZemaxDDE Client");
            ImGui::Separator();
            ImGui::Text("Version: %s", APP_FULL_VERSION);
            ImGui::Text("Built: %s", __DATE__ " " __TIME__);
            ImGui::Text("Git commit: %s", APP_GIT_COMMIT);
            ImGui::Text("Compiler: %s", __VERSION__);
            ImGui::Spacing();
            ImGui::Text("Author: d3m37r4");
            ImGui::Text("https://github.com/d3m37r4/ZemaxDDEClient");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}
