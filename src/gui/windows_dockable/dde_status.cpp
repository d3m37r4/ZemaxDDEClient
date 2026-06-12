#include <format>

#include "windows_dockable/dde_status.h"
#include "gui/constants.h"
#include "gui/theme_manager.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void DDEStatus::render(Logger& logger) {
        if (!m_connectionManager) return;
        if (!m_themeManager) return;

        ImGui::BeginChild("DDE Status Content",
            ImVec2(gui::DDE_STATUS_CONTENT_SIZE.x, gui::DDE_STATUS_CONTENT_SIZE.y),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        int connectionCount = 0;
        for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
            auto* conn = m_connectionManager->getConnection(i);
            if (conn && conn->isConnected) ++connectionCount;
        }
        int activeIdx = m_connectionManager->getActiveIndex();

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

            const auto& sem = m_themeManager->semantic();
            ImGui::PushStyleColor(ImGuiCol_Text, connected ? sem.success : sem.danger);
            ImGui::TextUnformatted(value);
            ImGui::PopStyleColor();
        }

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
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                if (connectionCount < DDEConnectionManager::MAX_CONNECTIONS) {
                    ImGui::Separator();
                    if (ImGui::Selectable("+ Connect to another Zemax...")) {
                        m_showConnectPopup = true;
                    }
                }

                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        bool connected = (activeIdx >= 0);

        // Connect/disconnect button uses semantic danger/success tokens (not ImGuiCol_Button*)
        // so the action remains visually unambiguous regardless of the active theme.
        const auto& sem = m_themeManager->semantic();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        connected ? sem.dangerButton        : sem.successButton);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, connected ? sem.dangerButtonHover   : sem.successButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  connected ? sem.dangerButtonActive  : sem.successButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

        if (ImGui::Button(connected ? "Disconnect from Zemax" : "Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            if (connected) {
                m_connectionManager->disconnect(activeIdx);
                logger.addLog("[DDE] Disconnected from Zemax");
            } else {
                m_showConnectPopup = true;
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();

        ImGui::EndChild();

        if (m_showConnectPopup && m_connectPopup) {
            m_connectPopup->render(m_showConnectPopup, logger);
        }
    }
}
