

#include "gui/components/gui_popups.h"

#include "gui/gui.h"
#include "app/app.h"

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
            std::string version = std::format("Version: {}", APP_FULL_VERSION);
            std::string built   = std::format("Built: {} {}", __DATE__, __TIME__);

            std::string commitUrl = std::format(
                "https://github.com/d3m37r4/ZemaxDDEClient/commit/{}",
                APP_GIT_COMMIT
            );

            ImGui::TextUnformatted(APP_NAME);
            ImGui::Separator();

            ImGui::TextUnformatted("GitHub Repository:");
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("github.com/d3m37r4/ZemaxDDEClient",
                                   "https://github.com/d3m37r4/ZemaxDDEClient");

            ImGui::TextUnformatted("Author:");
            ImGui::SameLine();
            ImGui::TextLinkOpenURL("Dmitry Isakov", "https://github.com/d3m37r4");

            ImGui::TextUnformatted(version.c_str());
            ImGui::TextUnformatted(built.c_str());
            ImGui::TextUnformatted("Git commit:");
            ImGui::SameLine();
            ImGui::TextLinkOpenURL(APP_GIT_COMMIT, commitUrl.c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float window_width = ImGui::GetWindowSize().x;
            float button_width = 120.0f;
            ImGui::SetCursorPosX((window_width - button_width) * 0.5f);
            if (ImGui::Button("OK", ImVec2(button_width, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}
