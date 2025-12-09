#include "gui/components/gui_debug_log.h"

#include "gui.h"

namespace {
    std::string collectLogContent() {
        std::string content;
        for (const auto& entry : logger.getLogs()) {
            content += entry;
            content += '\n';
        }
        return content;
    }

    void saveDebugLogToFile() {
        std::string logContent = collectLogContent();
        auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_DebugLog_Temp.txt", logContent);
        if (tempPathOpt) {
            ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
            logger.addLog(std::format("[GUI] Debug log saved to {}", tempPathOpt->string()));
        } else {
            logger.addLog("[GUI] Failed to create temporary file for debug log export");
        }
    }

    void copyDebugLogToClipboard() {
        std::string logContent = collectLogContent();
        logger.addLog("[GUI] Debug log copied to clipboard");
        ImGui::SetClipboardText(logContent.c_str());
    }
}

namespace gui {
    void GuiManager::renderDebugLog() {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(DEBUG_LOG_WIDTH_MIN, DEBUG_LOG_HEIGHT_MIN),  // min_size
            ImVec2(FLT_MAX, FLT_MAX)                            // max_size
        );

        if (!ImGui::Begin("Debug", nullptr)) {
            ImGui::End();
            return;
        }

        ImGui::BeginChild("DebugLogHeader", ImVec2(-1.0f, 0.0f), ImGuiChildFlags_AutoResizeY);
        if (ImGui::Button("Text")) {
            saveDebugLogToFile();
        }

        ImGui::SameLine();

        if (ImGui::Button("Copy to clipboard")) {
            copyDebugLogToClipboard();
        }

        ImGui::SameLine();

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Clear logs").x - ImGui::GetStyle().FramePadding.x * 2);
        if (ImGui::Button("Clear logs")) {
            logger.clearLogs();
        }
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

        ImGui::End();
    }
}
