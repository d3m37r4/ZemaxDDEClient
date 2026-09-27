#include "gui/popups/shortcuts_help_dialog.h"

#include <format>
#include <string>
#include <string_view>

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

        const char* currentSection = nullptr;
        bool tableOpen = false;
        int tableIndex = 0;
        for (int i = 0; i < shortcuts::kEntryCount; ++i) {
            const auto& entry = shortcuts::kEntries[i];
            if (currentSection == nullptr || std::string_view(currentSection) != entry.section) {
                if (tableOpen) {
                    ImGui::EndTable();
                    tableOpen = false;
                }
                currentSection = entry.section;
                ImGuiUtils::SectionHeader(currentSection);
                ImGui::BeginTable(std::format("##shortcuts_table_{}", tableIndex++).c_str(), 2,
                                  ImGuiTableFlags_SizingStretchProp);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed);
                tableOpen = true;
            }
            // Merge consecutive rows with the same action (F1 + Ctrl+/).
            std::string hint = entry.hint;
            while (i + 1 < shortcuts::kEntryCount
                   && std::string_view(shortcuts::kEntries[i + 1].section) == currentSection
                   && std::string_view(shortcuts::kEntries[i + 1].action) == entry.action) {
                hint += " / ";
                hint += shortcuts::kEntries[++i].hint;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(entry.action);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(hint.c_str());
        }
        if (tableOpen) {
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
