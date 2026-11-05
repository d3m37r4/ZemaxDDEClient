#include "gui/popups/clean_logs_confirm_dialog.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"

namespace gui {

    void CleanLogsConfirmDialog::open() noexcept {
        m_confirm = true;
        m_open = true;
    }

    void CleanLogsConfirmDialog::close() noexcept {
        m_confirm = false;
        m_open = false;
    }

    void CleanLogsConfirmDialog::render() {
        if (!m_confirm) return;

        if (m_open && !ImGui::IsPopupOpen(CLEAN_LOG_FILES_CONFIRM_POPUP_NAME)) {
            ImGui::OpenPopup(CLEAN_LOG_FILES_CONFIRM_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(CLEAN_LOG_FILES_CONFIRM_POPUP_MIN_SIZE.x, CLEAN_LOG_FILES_CONFIRM_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(CLEAN_LOG_FILES_CONFIRM_POPUP_DEFAULT_SIZE);

        if (!ImGuiUtils::BeginPopupModalEx(CLEAN_LOG_FILES_CONFIRM_POPUP_NAME, &m_confirm,
                                    ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        ImGui::BeginChild("##clean_logs_confirm_body",
                          ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                          ImGuiChildFlags_Borders);
        ImGui::TextUnformatted("This will delete all log files except the currently active log.");
        ImGui::EndChild();

        float cancelBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float cleanBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        const float totalW = cancelBtnW + cleanBtnW + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalW) * 0.5f);

        if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(cancelBtnW, 0))) {
            close();
        }

        ImGui::SameLine();

        if (m_themeManager) {
            const auto& sem = m_themeManager->semantic();
            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
        }

        if (ImGui::Button(ICON_FA_BROOM " Clean", ImVec2(cleanBtnW, 0))) {
            if (m_onConfirm) m_onConfirm();
            close();
        }

        if (m_themeManager) {
            ImGuiUtils::PopFilledButton();
        }

        ImGui::EndPopup();
    }

} // namespace gui
