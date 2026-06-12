#include "gui/popups/about_dialog.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
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
        if (m_open && !ImGui::IsPopupOpen(ABOUT_POPUP_NAME)) {
            ImGui::OpenPopup(ABOUT_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(ABOUT_POPUP_MIN_SIZE.x, ABOUT_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(ABOUT_POPUP_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal(ABOUT_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        const float footerH = ImGuiUtils::DpiScale(26.0f);

        ImGui::BeginChild("##about_body", ImVec2(0, -footerH), ImGuiChildFlags_Borders);

        std::string version = std::format("Version: {}", APP_FULL_VERSION);
        std::string built   = std::format("Built: {} {}", __DATE__, __TIME__);

        std::string commitUrl = std::format(
            "https://github.com/d3m37r4/ZemaxDDEClient/commit/{}",
            APP_GIT_COMMIT
        );

        ImGui::TextWrapped(
            "%s - Advanced analysis of optical systems using parameters "
            "retrieved from Zemax via DDE (Dynamic Data Exchange)",
            APP_NAME);
        ImGui::Spacing();    
        ImGui::Separator();

        ImGui::TextUnformatted("GitHub Repository:");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("github.com/d3m37r4/ZemaxDDEClient",
                               "https://github.com/d3m37r4/ZemaxDDEClient");

        ImGui::TextUnformatted("License:");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("MIT", "https://github.com/d3m37r4/ZemaxDDEClient/blob/main/LICENSE");

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
        float button_width = ImGuiUtils::DpiScale(120.0f);
        ImGui::SetCursorPosX((window_width - button_width) * 0.5f);
        if (ImGui::Button("OK", ImVec2(button_width, 0))) {
            close();
        }
        ImGui::EndPopup();
    }
}
