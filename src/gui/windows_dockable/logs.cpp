#include "windows_dockable/logs.h"

#include <format>
#include <string>

#include <windows.h>
#include <shellapi.h>

#include "gui/constants.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void Logs::render(Logger& logger, const ThemeManager* themeManager) {
        const auto& sem = themeManager->semantic();

        ImGui::BeginChild("LogsHeader", ImVec2(-1.0f, 0.0f), ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
        if (ImGui::Button(ICON_FA_FILE_EXPORT)) {
            std::string content;
            for (const auto& entry : logger.getLogs()) {
                content += entry;
                content += '\n';
            }
            auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_Logs_Temp.txt", content);
            if (tempPathOpt) {
                ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
                logger.addLog(std::format("[GUI] Log exported to {}", tempPathOpt->string()));
            } else {
                logger.addLog("[GUI] Failed to create temporary file for log export");
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Export logs to a temporary text file and open it");

        ImGui::SameLine();

        std::string content;
        for (const auto& entry : logger.getLogs()) {
            content += entry;
            content += '\n';
        }
        if (ImGuiUtils::CopyButton(ICON_FA_COPY "##logs_copy_all", content.c_str(),
                                   "Copy all log entries to clipboard")) {
            logger.addLog("[GUI] Logs copied to clipboard");
        }

        ImGui::SameLine();

        if (ImGui::Checkbox("Autoscroll", &m_autoscroll)) {
            m_jumpToBottom = m_autoscroll;
        }

        // Checkbox завершает строку (курсор у ImGui на новой) — возвращаемся,
        // иначе «мусорка» уедет на отдельный ряд (ловилось на скриншоте).
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(ICON_FA_TRASH).x - ImGui::GetStyle().FramePadding.x * 2);

        ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
        if (ImGui::Button(ICON_FA_TRASH)) {
            m_confirmDelete = true;
        }
        ImGuiUtils::PopFilledButton();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear all log entries (requires confirmation)");
        ImGui::EndChild();

        ImGui::BeginChild("LogsContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        const auto& logEntries = logger.getLogs();

        // Прилипание к низу: снимаем решение ДО отрисовки — до того, как новый
        // контент сдвинет скролл. Работает и после clearLogs() (список пуст,
        // максимум скролла 0 ⇒ «на дне»), и когда пользователь уехал наверх
        // читать историю — тогда автоскролл молчит, пока не вернётся.
        const float bottomSlack = ImGuiUtils::DpiScale(4.0f);
        const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - bottomSlack;

        for (const auto& entry : logEntries) {
            ImGui::Text(entry.c_str());
        }

        if (m_jumpToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_jumpToBottom = false;
        } else if (m_autoscroll && wasAtBottom) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        // Confirmation modal for the header "Clear" button. The in-memory log
        // and the log files on disk are different things, so it does not reuse
        // CleanLogsConfirmDialog (that one deletes folder files from Preferences).
        // The popup is owned by the root Logs window: open it here (the same
        // window that renders the modal), not inside the header child.
        if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_CONFIRM_POPUP_NAME)) {
            ImGui::OpenPopup(DELETE_LOG_CONFIRM_POPUP_NAME);
        }
        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(
            DELETE_LOG_CONFIRM_POPUP_MIN_SIZE.x, DELETE_LOG_CONFIRM_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(DELETE_LOG_CONFIRM_POPUP_DEFAULT_SIZE);
        if (ImGuiUtils::BeginPopupModalEx(DELETE_LOG_CONFIRM_POPUP_NAME, nullptr,
                                          ImGuiWindowFlags_NoCollapse)) {
            ImGui::BeginChild("##delete_log_confirm_body",
                              ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                              ImGuiChildFlags_Borders);
            ImGui::TextUnformatted("This will clear all entries in the on-screen log.");
            ImGui::TextUnformatted("Log files on disk are not removed.");
            ImGui::EndChild();

            const float clearBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            const float cancelBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            const float totalW = cancelBtnW + clearBtnW + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalW) * 0.5f);

            if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(cancelBtnW, 0))) {
                m_confirmDelete = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover,
                                         sem.dangerButtonActive, sem.onAccent);
            if (ImGui::Button(ICON_FA_TRASH " Clear", ImVec2(clearBtnW, 0))) {
                logger.clearLogs();
                m_confirmDelete = false;
                ImGui::CloseCurrentPopup();
            }
            ImGuiUtils::PopFilledButton();

            ImGui::EndPopup();
        } else if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_CONFIRM_POPUP_NAME)) {
            // Closed through the app-wide close-popup shortcut (Esc) — drop the
            // pending request so it does not reopen on the next frame.
            m_confirmDelete = false;
        }
    }
}
