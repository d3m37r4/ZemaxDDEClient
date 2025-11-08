#include "gui.h"

namespace gui {
    void GuiManager::renderDDEStatusFrame() {
        ImGui::BeginChild("DDE Status Frame", 
            ImVec2(DDE_STATUS_FRAME_WIDTH, DDE_STATUS_FRAME_HEIGHT), 
            ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        const char* label = "Zemax DDE Status:";
        const char* value = isDdeInitialized() ? "Connected" : "Disconnected";

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float labelWidth = ImGui::CalcTextSize(label).x;
        float valueWidth = ImGui::CalcTextSize(value).x;
        float totalWidth = labelWidth + valueWidth + 4.0f;
        float offsetX = (availableWidth - totalWidth) * 0.5f;
        if (offsetX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

        ImGui::TextUnformatted(label);
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, isDdeInitialized() ? DDE_STATUS_COLOR_CONNECTED : DDE_STATUS_COLOR_DISCONNECTED);
        ImGui::TextUnformatted(value);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, isDdeInitialized() ? DDE_BUTTON_DISCONNECT_COLOR_NORMAL : DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isDdeInitialized() ? DDE_BUTTON_DISCONNECT_COLOR_HOVER : DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, isDdeInitialized() ? DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        if (ImGui::Button(isDdeInitialized() ? "Disconnect from Zemax" : "Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            try {
                if (!isDdeInitialized()) {
                    zemaxDDEClient->initiateDDE();
                } else {
                    zemaxDDEClient->terminateDDE();
                }
            #ifdef DEBUG_LOG
                logger.addLog("[GUI] " + std::string(isDdeInitialized() ? "Connected to Zemax" : "Disconnected from Zemax"));
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
