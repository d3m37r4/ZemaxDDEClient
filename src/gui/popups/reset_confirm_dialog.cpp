#include "gui/popups/reset_confirm_dialog.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
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

        if (!ImGuiUtils::BeginPopupModalEx(RESET_CONFIRM_POPUP_NAME, &m_confirmReset,
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
        if (m_themeManager) {
            const auto& sem = m_themeManager->semantic();
            ImGui::PushStyleColor(ImGuiCol_Button,        sem.neutralButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  sem.neutralButtonHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   sem.neutralButtonActive);
        }

        if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(cancelBtnW, 0))) {
            close();
        }

        if (m_themeManager) {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        if (m_themeManager) {
            const auto& sem = m_themeManager->semantic();
            ImGui::PushStyleColor(ImGuiCol_Button,        sem.dangerButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  sem.dangerButtonActive);
            ImGui::PushStyleColor(ImGuiCol_Text,          sem.onAccent);
        }

        if (ImGui::Button(ICON_FA_ROTATE_LEFT " Reset", ImVec2(resetBtnW, 0))) {
            if (m_onReset) m_onReset();
            close();
        }

        if (m_themeManager) {
            ImGui::PopStyleColor(4);
        }

        ImGui::EndPopup();
    }

} // namespace gui
