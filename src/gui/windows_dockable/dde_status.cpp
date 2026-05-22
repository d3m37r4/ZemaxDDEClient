#include <format>

#include "windows_dockable/dde_status.h"
#include "gui/constants.h"
#include "app/zemax_window_enumerator.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void DDEStatus::render(Logger& logger) {
        if (!m_connectionManager) return;

        ImGui::BeginChild("DDE Status Content",
            ImVec2(gui::DDE_STATUS_CONTENT_WIDTH, gui::DDE_STATUS_CONTENT_HEIGHT),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        int connectionCount = 0;
        for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
            auto* conn = m_connectionManager->getConnection(i);
            if (conn && conn->isConnected) ++connectionCount;
        }
        int activeIdx = m_connectionManager->getActiveIndex();

        // --- Connection status indicator ---
        {
            const bool connected = (activeIdx >= 0);
            const char* label = "Zemax DDE Status:";
            const char* value = connected ? "Connected" : "Disconnected";

            float availableWidth = ImGui::GetContentRegionAvail().x;
            float labelWidth = ImGui::CalcTextSize(label).x;
            float valueWidth = ImGui::CalcTextSize(value).x;
            float totalWidth = labelWidth + valueWidth + 4.0f;
            float offsetX = (availableWidth - totalWidth) * 0.5f;
            if (offsetX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

            ImGui::TextUnformatted(label);
            ImGui::SameLine(0.0f, 4.0f);

            ImGui::PushStyleColor(ImGuiCol_Text, connected ? gui::DDE_STATUS_COLOR_CONNECTED : gui::DDE_STATUS_COLOR_DISCONNECTED);
            ImGui::TextUnformatted(value);
            ImGui::PopStyleColor();
        }

        // --- Active target dropdown (only when connected) ---
        if (connectionCount > 0) {
            ImGui::Separator();
            ImGui::Text("Active Target:");

            std::string preview;
            for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
                auto* conn = m_connectionManager->getConnection(i);
                if (!conn || !conn->isConnected) continue;
                std::string title(conn->serverTitle.begin(), conn->serverTitle.end());
                std::string itemLabel = std::format("[{}] {}", i, title);
                if (i == activeIdx) {
                    preview = itemLabel;
                }
            }

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##TargetCombo", preview.c_str())) {
                for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
                    auto* conn = m_connectionManager->getConnection(i);
                    if (!conn || !conn->isConnected) continue;
                    std::string title(conn->serverTitle.begin(), conn->serverTitle.end());
                    std::string itemLabel = std::format("[{}] {}", i, title);

                    bool isSelected = (i == activeIdx);
                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        m_connectionManager->setActiveConnection(i);
                        logger.addLog(std::format("[DDE] Switched active target to [{}] {}", i, title));
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        // --- Connect / Disconnect button ---
        bool connected = (activeIdx >= 0);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_NORMAL : gui::DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_HOVER : gui::DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : gui::DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

        if (ImGui::Button(connected ? "Disconnect" : "Connect to Zemax...", ImVec2(-1.0f, 0.0f))) {
            if (connected) {
                m_connectionManager->disconnect(activeIdx);
                logger.addLog("[DDE] Disconnected from Zemax");
            } else {
                m_showConnectPopup = true;
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();

        // --- Connect popup ---
        if (m_showConnectPopup) {
            renderConnectPopup(logger);
        }
    }

    void DDEStatus::renderConnectPopup(Logger& logger) {
        ImGui::OpenPopup("Connect to Zemax");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal("Connect to Zemax", &m_showConnectPopup, ImGuiWindowFlags_NoResize)) {
            ImGui::TextWrapped("Select a Zemax window to connect to:");

            ImGui::Separator();

            // Refresh button
            if (ImGui::Button("Refresh")) {
                m_selectedWindowIndex = -1;
            }

            ImGui::SameLine();

            // Scan & list windows
            auto windows = app::ZemaxWindowEnumerator::enumerate();

            ImGui::BeginChild("ZemaxWindowList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), ImGuiChildFlags_Borders);

            for (size_t i = 0; i < windows.size(); ++i) {
                const auto& wnd = windows[i];
                std::string title(wnd.title.begin(), wnd.title.end());
                std::string label = std::format("{} (PID: {})", title, wnd.pid);

                if (ImGui::Selectable(label.c_str(), m_selectedWindowIndex == static_cast<int>(i))) {
                    m_selectedWindowIndex = static_cast<int>(i);
                }
            }

            if (windows.empty()) {
                ImGui::TextDisabled("No Zemax windows found");
            }

            ImGui::EndChild();

            // Bottom buttons
            bool canConnect = (m_selectedWindowIndex >= 0 && m_selectedWindowIndex < static_cast<int>(windows.size()));

            if (!canConnect) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Connect", ImVec2(120, 0))) {
                const auto& wnd = windows[m_selectedWindowIndex];
                int slot = m_connectionManager->connectToZemax(wnd.hwnd, wnd.title);
                if (slot >= 0) {
                    std::string title(wnd.title.begin(), wnd.title.end());
                    logger.addLog(std::format("[DDE] Connected to Zemax: {} (PID: {})", title, wnd.pid));
                    m_showConnectPopup = false;
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
                m_showConnectPopup = false;
                m_selectedWindowIndex = -1;
            }

            ImGui::EndPopup();
        }
    }
}
