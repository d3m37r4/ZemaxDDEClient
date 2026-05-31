#include <format>

#include "connect_dde.h"
#include "app/zemax_window_enumerator.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void ConnectDDEPopup::render(bool& showFlag, Logger& logger) {
        if (!showFlag) return;

        constexpr const char* POPUP_TITLE = "Connect to Zemax – select a window";

        ImGui::OpenPopup(POPUP_TITLE);
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGuiUtils::SetDpiScaledWindowConstraints(CONNECT_DDE_POPUP_WIDTH_MIN, CONNECT_DDE_POPUP_HEIGHT_MIN);
        // ImGui::SetNextWindowSize(ImVec2(920, 400), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal(POPUP_TITLE, &showFlag, ImGuiWindowFlags_None)) {
            auto windows = app::ZemaxWindowEnumerator::enumerate();

            ImGui::BeginChild("ZemaxWindowList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), ImGuiChildFlags_Borders);

            for (size_t i = 0; i < windows.size(); ++i) {
                const auto& wnd = windows[i];
                std::string title(wnd.title.begin(), wnd.title.end());
                std::string label = std::format("[PID: {}] {}", wnd.pid, title);

                if (ImGui::Selectable(label.c_str(), m_selectedWindowIndex == static_cast<int>(i))) {
                    m_selectedWindowIndex = static_cast<int>(i);
                }
            }

            if (windows.empty()) {
                ImGui::TextDisabled("No Zemax windows found");
            }

            ImGui::EndChild();

            bool canConnect = (m_selectedWindowIndex >= 0 && m_selectedWindowIndex < static_cast<int>(windows.size()));

            if (!canConnect) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Connect", ImVec2(120, 0))) {
                const auto& wnd = windows[m_selectedWindowIndex];
                int slot = m_connectionManager->connectToZemax(wnd.hwnd, wnd.title);
                if (slot >= 0) {
                    showFlag = false;
                    m_selectedWindowIndex = -1;
                } else {
                    std::string title(wnd.title.begin(), wnd.title.end());
                    logger.addLog(std::format("[DDE] Failed to connect to: {}", title));
                }
            }

            if (!canConnect) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showFlag = false;
                m_selectedWindowIndex = -1;
            }

            ImGui::SameLine();
            float remainingWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remainingWidth - 120.0f);
            if (ImGui::Button("Refresh", ImVec2(120, 0))) {
                m_selectedWindowIndex = -1;
            }

            ImGui::EndPopup();
        }
    }
}
