#include "gui/popups/shortcuts_help_dialog.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/shortcuts.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void ShortcutsHelpDialog::open() noexcept {
        m_open = true;
    }

    void ShortcutsHelpDialog::close() noexcept {
        m_open = false;
    }

    void ShortcutsHelpDialog::render() {
        if (m_open && !ImGui::IsPopupOpen(SHORTCUTS_POPUP_NAME)) {
            ImGui::OpenPopup(SHORTCUTS_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(SHORTCUTS_POPUP_MIN_SIZE.x, SHORTCUTS_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(SHORTCUTS_POPUP_DEFAULT_SIZE);

        if (!ImGuiUtils::BeginPopupModalEx(SHORTCUTS_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        ImGui::BeginChild("##shortcuts_body", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
        if (ImGui::BeginTable("##shortcuts_table", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed);
            for (int i = 0; i < shortcuts::kEntryCount; ++i) {
                const auto& entry = shortcuts::kEntries[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(entry.action);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(entry.hint);
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        float okBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - okBtnW) * 0.5f);
        if (ImGui::Button("OK", ImVec2(okBtnW, 0))) {
            close();
        }
        ImGui::EndPopup();
    }
}
