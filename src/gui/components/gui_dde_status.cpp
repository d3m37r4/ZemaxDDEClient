#include <stdexcept>
#include <string>
#include "components/gui_dde_status.h"
#include "dde/dde_zemax_client.h"
#include "gui.h"

namespace gui {
    void GuiManager::renderDDEStatusFrame() {
        ImGui::BeginChild("DDE Status Frame", ImVec2(DDE_STATUS_FRAME_WIDTH, DDE_STATUS_FRAME_HEIGHT), 
        ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::Text("Zemax DDE Status:");
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, dde_initialized ? DDE_STATUS_COLOR_CONNECTED : DDE_STATUS_COLOR_DISCONNECTED);
        ImGui::Text(dde_initialized ? "Connected" : "Disconnected");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_NORMAL : DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_HOVER : DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        if (ImGui::Button(dde_initialized ? "Disconnect from Zemax" : "Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            try {
                if (!dde_initialized) {
                    zemaxDDEClient->initiateDDE();
                    dde_initialized = true;
                #ifdef DEBUG_LOG
                    logger.addLog("[GUI] Connected to Zemax");
                #endif
                } else {
                    zemaxDDEClient->terminateDDE();
                    dde_initialized = false;
                #ifdef DEBUG_LOG
                    logger.addLog("[GUI] Disconnected from Zemax");
                #endif
                }
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
