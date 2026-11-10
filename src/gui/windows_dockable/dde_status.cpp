#include <format>

#include "windows_dockable/dde_status.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/shortcuts.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "dde/client.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void DDEStatus::toggleConnection(Logger& logger) {
        if (!m_connectionManager) return;
        const int activeIdx = m_connectionManager->getActiveIndex();
        // One action per state: with a slot the button disconnects it, otherwise
        // it opens the picker. The old "acknowledge the lost flag" branch could
        // log a disconnect while nothing was connected; the loss flag is consumed
        // by renderConnectionLostPopup, so it is not needed here.
        if (activeIdx >= 0) {
            m_connectionManager->disconnect(activeIdx);
            logger.addLog("[DDE] Disconnected from Zemax");
            return;
        }
        m_connectPopup->open();
    }

    void DDEStatus::openConnectPopup() {
        m_connectPopup->open();
    }

    void DDEStatus::closeConnectPopup() noexcept {
        m_connectPopup->close();
    }

    void DDEStatus::render(Logger& logger) {
        if (!m_connectionManager) return;
        if (!m_themeManager) return;

        ImGui::BeginChild("DDE Status Content",
            ImVec2(gui::DDE_STATUS_CONTENT_SIZE.x, gui::DDE_STATUS_CONTENT_SIZE.y),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        int connectionCount = 0;
        for (int i = 0; i < m_connectionManager->getMaxConnections(); ++i) {
            auto* conn = m_connectionManager->getConnection(i);
            if (conn && conn->isConnected()) ++connectionCount;
        }
        int activeIdx = m_connectionManager->getActiveIndex();
        bool connectionLost = m_connectionManager->hasConnectionLost();
        // One derived state for the whole panel: the status label, the button and
        // the status bar agree. A pending "connection lost" flag with no slot is
        // not a connection, so the button offers "Connect".
        const bool hasSlot = activeIdx >= 0;
        const bool connected = hasSlot && !connectionLost;

        {
            const char* label = "Zemax DDE Status:";
            const char* value;
            const char* icon;
            ImVec4 valueColor;

            const auto& sem = m_themeManager->semantic();
            if (connectionLost) {
                value = "Connection Lost";
                icon = ICON_FA_TRIANGLE_EXCLAMATION;
                valueColor = sem.warning;
            } else if (connected) {
                value = "Connected";
                icon = ICON_FA_CIRCLE_CHECK;
                valueColor = sem.success;
            } else {
                value = "Disconnected";
                icon = ICON_FA_CIRCLE_XMARK;
                valueColor = sem.danger;
            }

            float availableWidth = ImGui::GetContentRegionAvail().x;
            float labelWidth = ImGui::CalcTextSize(label).x;
            float valueWidth = ImGui::CalcTextSize(value).x;
            float iconWidth = ImGui::CalcTextSize(icon).x;
            float totalWidth = labelWidth + valueWidth + iconWidth + ImGuiUtils::DpiScale(DDE_STATUS_ELEMENT_GAP) * 2;
            float offsetX = (availableWidth - totalWidth) * 0.5f;
            if (offsetX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

            ImGui::TextUnformatted(label);
            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(DDE_STATUS_ELEMENT_GAP));

            ImGui::PushStyleColor(ImGuiCol_Text, valueColor);
            ImGui::TextUnformatted(value);
            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(DDE_STATUS_ELEMENT_GAP));
            ImGui::TextUnformatted(icon);
            ImGui::PopStyleColor();
        }

        if (connectionCount > 0) {
            ImGui::Separator();
            ImGui::Text("Active Target:");
            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(DDE_STATUS_ELEMENT_GAP));
            ImGuiUtils::HelpMarker("Switch target: F6 (next), Alt+1 / Alt+2 (direct)");
            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(DDE_STATUS_ELEMENT_GAP));

            std::string preview;
            for (int i = 0; i < m_connectionManager->getMaxConnections(); ++i) {
                auto* conn = m_connectionManager->getConnection(i);
                if (!conn || !conn->isConnected()) continue;
                std::string title = ZemaxDDE::wstring_to_utf8(conn->serverTitle);
                std::string itemLabel = std::format("[{}] {}", i, title);
                if (i == activeIdx) {
                    preview = itemLabel;
                }
            }

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##TargetCombo", preview.c_str())) {
                for (int i = 0; i < m_connectionManager->getMaxConnections(); ++i) {
                    auto* conn = m_connectionManager->getConnection(i);
                    if (!conn || !conn->isConnected()) continue;
                    std::string title = ZemaxDDE::wstring_to_utf8(conn->serverTitle);

                    bool slotBusy = false;
                    if (conn->client) {
                        auto* monitor = conn->client->getOperationMonitor();
                        if (monitor) {
                            for (const auto& op : monitor->getOperations()) {
                                if (op.status == ZemaxDDE::OperationStatus::Pending ||
                                    op.status == ZemaxDDE::OperationStatus::InFlight) {
                                    slotBusy = true;
                                    break;
                                }
                            }
                        }
                    }

                    std::string itemLabel = std::format("[{}] {}", i, title);

                    bool isSelected = (i == activeIdx);
                    if (slotBusy) {
                        const float eqH = ImGui::GetTextLineHeight() * 0.75f;
                        ImGuiUtils::BusyEqualizer(eqH * 1.4f, eqH,
                            m_themeManager->semantic().info);
                        ImGui::SameLine();
                    }
                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        m_connectionManager->setActiveConnection(i);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                if (connectionCount < m_connectionManager->getMaxConnections()) {
                    ImGui::Separator();
                    if (ImGui::Selectable(ICON_FA_PLUS " Connect to another Zemax...")) {
                        m_connectPopup->open();
                    }
                }

                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        // Connect/disconnect button uses semantic danger/success tokens (not ImGuiCol_Button*)
        // so the action remains visually unambiguous regardless of the active theme.
        const auto& sem = m_themeManager->semantic();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, ImGuiUtils::DpiScale(DDE_STATUS_BTN_VPAD)));
        ImGuiUtils::PushFilledButton(hasSlot ? sem.dangerButton : sem.successButton,
                                     hasSlot ? sem.dangerButtonHover : sem.successButtonHover,
                                     hasSlot ? sem.dangerButtonActive : sem.successButtonActive,
                                     sem.onAccent);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

        if (ImGui::Button(hasSlot ? ICON_FA_PLUG_CIRCLE_XMARK " Disconnect from Zemax" : ICON_FA_PLUG " Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            toggleConnection(logger);
        }

        ImGui::PopStyleVar();
        ImGuiUtils::PopFilledButton();
        ImGui::PopStyleVar();

        // Tooltip after Pop: it must render in the default text color, not the
        // white on-accent color pushed for the button above (invisible on Light).
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", shortcuts::tooltipFor(
                hasSlot ? "dde-disconnect" : "dde-connect").c_str());
        }

        ImGui::EndChild();

        m_connectPopup->render();
    }
}
