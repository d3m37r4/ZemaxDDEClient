#include <stdexcept>
#include "windows_dockable/dde_status.h"
#include "gui/gui.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void DDEStatus::render(Logger& logger) {
        ImGui::BeginChild("DDE Status Content",
            ImVec2(gui::DDE_STATUS_CONTENT_WIDTH, gui::DDE_STATUS_CONTENT_HEIGHT),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        const char* label = "Zemax DDE Status:";
        const char* value = m_ddeClient && m_ddeClient->isConnected() ? "Connected" : "Disconnected";

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float labelWidth = ImGui::CalcTextSize(label).x;
        float valueWidth = ImGui::CalcTextSize(value).x;
        float totalWidth = labelWidth + valueWidth + 4.0f;
        float offsetX = (availableWidth - totalWidth) * 0.5f;
        if (offsetX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

        ImGui::TextUnformatted(label);
        ImGui::SameLine(0.0f, 4.0f);

        bool connected = m_ddeClient && m_ddeClient->isConnected();
        ImGui::PushStyleColor(ImGuiCol_Text, connected ? gui::DDE_STATUS_COLOR_CONNECTED : gui::DDE_STATUS_COLOR_DISCONNECTED);
        ImGui::TextUnformatted(value);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_NORMAL : gui::DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_HOVER : gui::DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, connected ? gui::DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : gui::DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

        if (ImGui::Button(connected ? "Disconnect from Zemax" : "Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            try {
                if (!connected) {
                    m_ddeClient->initiateDDE();
                } else {
                    m_ddeClient->terminateDDE();
                }
                
                #ifdef DEBUG_LOG
                logger.addLog("[GUI] " + std::string(connected ? "Connected to Zemax" : "Disconnected from Zemax"));
                #endif
            } catch (const std::runtime_error& e) {
                logger.addLog("[GUI] DDE connection failed: " + std::string(e.what()));
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }
}
