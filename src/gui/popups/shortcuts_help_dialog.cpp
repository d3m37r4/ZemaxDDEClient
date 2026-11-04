#include "gui/popups/shortcuts_help_dialog.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

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

        const auto sections = shortcuts::buildDisplaySections();

        float maxHintWidth = 0.0f;
        for (const auto& section : sections) {
            for (const auto& row : section.rows) {
                maxHintWidth = std::max(maxHintWidth, ImGui::CalcTextSize(row.hint.c_str()).x);
            }
        }
        const float hintColumnWidth =
            maxHintWidth + ImGui::GetStyle().FramePadding.x * 2.0f;

        int tableIndex = 0;
        for (const auto& section : sections) {
            ImGuiUtils::SectionHeader(section.name);
            ImGui::BeginTable(std::format("##shortcuts_table_{}", tableIndex++).c_str(), 2,
                              ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed, hintColumnWidth);
            for (const auto& row : section.rows) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(row.action);
                ImGui::TableSetColumnIndex(1);
                const float textWidth = ImGui::CalcTextSize(row.hint.c_str()).x;
                const float available = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     std::max(0.0f, available - textWidth));
                ImGui::TextUnformatted(row.hint.c_str());
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
