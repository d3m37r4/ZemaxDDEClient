#include <string>
#include "lib/imgui/imgui.h"
#include "components/gui_debug_log.h"
#include "gui.h"

namespace gui {
    void GuiManager::renderDebugLogFrame() {
        ImGui::BeginChild("DebugLogHeader", ImVec2(0, ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::BeginChild("DebugLabel", ImVec2(ImGui::GetWindowWidth() * 0.5f, 0), false);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "DEBUG");
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("DebugCopyButton", ImVec2(0, 0), false);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(COPY_CLIPBOARD_BUTTON_TITLE).x - ImGui::GetStyle().FramePadding.x * 2);
        if (ImGui::Button(COPY_CLIPBOARD_BUTTON_TITLE)) {
            std::string log_text;
            for (const auto& log : logger.getLogs()) {
                log_text += log + "\n";
            }
            logger.addLog("[GUI] Debug log copied to clipboard");
            ImGui::SetClipboardText(log_text.c_str());
        }
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::BeginChild("DebugLogContent", ImVec2(0, DEBUG_LOG_HEIGHT - ImGui::GetFrameHeightWithSpacing()), true);
        static size_t last_log_size = 0;
        const auto& logEntries = logger.getLogs();
        for (const auto& log : logEntries) {
            ImGui::Text(log.c_str());
        }
        if (logEntries.size() > last_log_size) {
            ImGui::SetScrollHereY(1.0f);
            last_log_size = logEntries.size();
        }
        ImGui::EndChild();
    }
}
