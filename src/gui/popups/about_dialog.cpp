#include "gui/popups/about_dialog.h"
#include "lib/imgui/imgui.h"
#include "app/app.h"
#include "version.h"
#include <string>
#include <format>

namespace gui {
    void AboutDialog::open() noexcept {
        m_open = true;
    }

    void AboutDialog::close() noexcept {
        m_open = false;
    }

    void AboutDialog::render() {
        if (m_open && !ImGui::IsPopupOpen("About")) {
            ImGui::OpenPopup("About");
        }

        ImGui::SetNextWindowSize(ImVec2(440.0f, 240.0f), ImGuiCond_FirstUseEver);

        if (!ImGui::BeginPopupModal("About", &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        const float footerH = 40.0f;

        ImGui::BeginChild("##about_body", ImVec2(0, -footerH), ImGuiChildFlags_Borders);

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

        ImGui::EndChild();

        float window_width = ImGui::GetWindowSize().x;
        float button_width = 120.0f;
        ImGui::SetCursorPosX((window_width - button_width) * 0.5f);
        if (ImGui::Button("OK", ImVec2(button_width, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
