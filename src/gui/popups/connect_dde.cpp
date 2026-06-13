#include <format>

#include "connect_dde.h"
#include "app/zemax_window_enumerator.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void ConnectDDEPopup::open() noexcept {
        m_open = true;
    }

    void ConnectDDEPopup::close() noexcept {
        m_open = false;
    }

    void ConnectDDEPopup::render() {
        if (!m_open) return;
        if (!m_logger) return;

        if (!ImGui::IsPopupOpen(CONNECT_DDE_POPUP_NAME)) {
            ImGui::OpenPopup(CONNECT_DDE_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(CONNECT_DDE_POPUP_MIN_SIZE.x, CONNECT_DDE_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(CONNECT_DDE_POPUP_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal(CONNECT_DDE_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        auto windows = app::ZemaxWindowEnumerator::enumerate();

        ImGui::BeginChild("ZemaxWindowList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

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

        float connectBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float cancelBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float refreshBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);

        if (!canConnect) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Connect", ImVec2(connectBtnW, 0))) {
            const auto& wnd = windows[m_selectedWindowIndex];
            int slot = m_connectionManager->connectToZemax(wnd.hwnd, wnd.title);
            if (slot >= 0) {
                m_open = false;
                m_selectedWindowIndex = -1;
            } else {
                std::string title(wnd.title.begin(), wnd.title.end());
                m_logger->addLog(std::format("[DDE] Failed to connect to: {}", title));
            }
        }

        if (!canConnect) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(cancelBtnW, 0))) {
            m_open = false;
            m_selectedWindowIndex = -1;
        }

        ImGui::SameLine();
        float remainingWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remainingWidth - refreshBtnW);
        if (ImGui::Button("Refresh", ImVec2(refreshBtnW, 0))) {
            m_selectedWindowIndex = -1;
        }

        ImGui::EndPopup();
    }
}
