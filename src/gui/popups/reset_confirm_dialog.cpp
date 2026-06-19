#include "gui/popups/reset_confirm_dialog.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

namespace gui {

    void ResetConfirmDialog::open() noexcept {
        m_confirmReset = true;
        m_open = true;
    }

    void ResetConfirmDialog::close() noexcept {
        m_confirmReset = false;
        m_open = false;
    }

    void ResetConfirmDialog::render() {
        if (!m_confirmReset) return;

        if (m_open && !ImGui::IsPopupOpen(RESET_CONFIRM_POPUP_NAME)) {
            ImGui::OpenPopup(RESET_CONFIRM_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(RESET_CONFIRM_POPUP_MIN_SIZE.x, RESET_CONFIRM_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(RESET_CONFIRM_POPUP_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal(RESET_CONFIRM_POPUP_NAME, &m_confirmReset,
                                    ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        ImGui::BeginChild("##reset_confirm_body",
                          ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                          ImGuiChildFlags_Borders);
        ImGui::TextUnformatted("This will reset all preferences to their factory defaults.");
        ImGui::TextUnformatted("Unsaved changes will be lost.");
        ImGui::EndChild();

        float cancelBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float resetBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        const float totalW = cancelBtnW + resetBtnW + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalW) * 0.5f);
        if (ImGui::Button("Cancel", ImVec2(cancelBtnW, 0))) {
            close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(resetBtnW, 0))) {
            if (m_onReset) m_onReset();
            close();
        }

        ImGui::EndPopup();
    }

} // namespace gui
