#include "gui/popups/connection_lost_dialog.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void ConnectionLostDialog::open(const std::string& reason) {
        m_reason = reason;
        m_open = true;
    }

    void ConnectionLostDialog::close() noexcept {
        m_open = false;
    }

    void ConnectionLostDialog::render() {
        if (m_open && !ImGui::IsPopupOpen("Connection Lost")) {
            ImGui::OpenPopup("Connection Lost");
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(CONNECTION_LOST_POPUP_MIN_SIZE.x, CONNECTION_LOST_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(CONNECTION_LOST_POPUP_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal("Connection Lost", &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        ImGui::BeginChild("##connection_lost_body", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

        ImGui::TextWrapped("Connection to Zemax has been lost.");
        ImGui::TextWrapped("%s", m_reason.c_str());

        ImGui::EndChild();

        float okBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - okBtnW) * 0.5f);
        if (ImGui::Button("OK", ImVec2(okBtnW, 0))) {
            close();
        }

        ImGui::EndPopup();
    }
}
