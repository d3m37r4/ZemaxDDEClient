#include "windows_dockable/debug_log.h"

#include <format>
#include <string>

#include <windows.h>
#include <shellapi.h>

#include "gui/constants.h"
#include "gui/utils.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void DebugLog::render(Logger& logger, const ThemeManager* themeManager) {
        ImGui::BeginChild("DebugLogHeader", ImVec2(-1.0f, 0.0f), ImGuiChildFlags_AutoResizeY);
        if (ImGui::Button(ICON_FA_FILE_EXPORT " Text")) {
            std::string content;
            for (const auto& entry : logger.getLogs()) {
                content += entry;
                content += '\n';
            }
            auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_DebugLog_Temp.txt", content);
            if (tempPathOpt) {
                ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
                logger.addLog(std::format("[GUI] Debug log saved to {}", tempPathOpt->string()));
            } else {
                logger.addLog("[GUI] Failed to create temporary file for debug log export");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_COPY " Copy to clipboard")) {
            std::string content;
            for (const auto& entry : logger.getLogs()) {
                content += entry;
                content += '\n';
            }
            logger.addLog("[GUI] Debug log copied to clipboard");
            ImGui::SetClipboardText(content.c_str());
        }

        ImGui::SameLine();

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(ICON_FA_TRASH " Clear logs").x - ImGui::GetStyle().FramePadding.x * 2);

        const auto& sem = themeManager->semantic();
        ImGui::PushStyleColor(ImGuiCol_Button,        sem.dangerButton);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  sem.dangerButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
        if (ImGui::Button(ICON_FA_TRASH " Clear logs")) {
            logger.clearLogs();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndChild();

        ImGui::BeginChild("DebugLogContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        static size_t lastLogSize = 0;
        const auto& logEntries = logger.getLogs();

        for (const auto& entry : logEntries) {
            ImGui::Text(entry.c_str());
        }

        if (logEntries.size() > lastLogSize) {
            ImGui::SetScrollHereY(1.0f);
            lastLogSize = logEntries.size();
        }
        ImGui::EndChild();
    }
}
