#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "components/gui_dde_status.h"
#include "dde_client.h"
#include "gui.h"

namespace gui {
    void GuiManager::renderDDEStatusFrame() {
        ImGui::BeginChild("DDE Status Frame", ImVec2(DDE_STATUS_FRAME_WIDTH, DDE_STATUS_FRAME_HEIGHT), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::Text("Zemax DDE Status:");
        ImGui::SameLine(0, 1.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, dde_initialized ? DDE_STATUS_COLOR_CONNECTED : DDE_STATUS_COLOR_DISCONNECTED);
        ImGui::Text(dde_initialized ? "Connected" : "Disconnected");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_NORMAL : DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_HOVER : DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, dde_initialized ? DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        if (ImGui::Button(dde_initialized ? "Disconnect from Zemax (DDE)" : "Connect to Zemax (DDE)", ImVec2(-1, -1))) {
            try {
                if (!dde_initialized) {
                    ZemaxDDE::initiateDDE(hwndDDE);
                    dde_initialized = true;
                    logger.addLog("DDE connection established successfully");
                } else {
                    ZemaxDDE::terminateDDE();
                    dde_initialized = false;
                    logger.addLog("DDE connection terminated");
                }
            } catch (const std::runtime_error& e) {
                logger.addLog((std::string("DDE Error: ") + e.what()).c_str());
                setErrorMsg(e.what());
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }
}
