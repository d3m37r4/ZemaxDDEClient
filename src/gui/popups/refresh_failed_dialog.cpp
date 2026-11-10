#include "gui/popups/refresh_failed_dialog.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"
// GetTopMostAndVisiblePopupModal: never stack this modal over another one.
#include "imgui_internal.h"

namespace gui {
    void RefreshFailedDialog::requestOpen(const std::string& reason) {
        // One window at a time: later failures are only logged by the caller.
        if (m_open || m_pending) return;
        m_reason = reason;
        m_pending = true;
    }

    void RefreshFailedDialog::close() noexcept {
        m_open = false;
        m_pending = false;
    }

    void RefreshFailedDialog::render() {
        if (m_pending) {
            // Opening a modal while another one is active would steal its stack
            // slot; keep the request pending until the user closes the other one.
            if (ImGui::GetTopMostAndVisiblePopupModal() != nullptr) {
                return;
            }
            m_pending = false;
            m_open = true;
            ImGui::OpenPopup(REFRESH_FAILED_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(
            REFRESH_FAILED_POPUP_MIN_SIZE.x, REFRESH_FAILED_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(REFRESH_FAILED_POPUP_DEFAULT_SIZE);

        if (!ImGuiUtils::BeginPopupModalEx(REFRESH_FAILED_POPUP_NAME, &m_open,
                                           ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        ImGui::BeginChild("##refresh_failed_body",
                          ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
        ImGui::TextWrapped("%s", m_reason.c_str());
        ImGui::Spacing();
        ImGui::TextWrapped("Check the model in OpticStudio: glass catalogs, solves, "
                           "multi-configuration and surface parameters. Then run the "
                           "calculation again.");
        ImGui::EndChild();

        const float okBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - okBtnW) * 0.5f);
        if (ImGui::Button("OK", ImVec2(okBtnW, 0))) {
            close();
        }
        ImGui::EndPopup();
    }
}
