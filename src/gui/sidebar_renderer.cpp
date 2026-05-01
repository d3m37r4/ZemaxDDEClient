#include <stdexcept>
#include "gui/sidebar_renderer.h"
#include "gui/gui.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace {
    inline constexpr ImVec4 DDE_STATUS_COLOR_CONNECTED    = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    inline constexpr ImVec4 DDE_STATUS_COLOR_DISCONNECTED = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_NORMAL  = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_HOVER   = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_ACTIVE  = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_NORMAL = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_HOVER  = ImVec4(0.7f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_ACTIVE = ImVec4(0.3f, 0.0f, 0.0f, 1.0f);
}

namespace gui {
    void SidebarRenderer::renderSidebar(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger) {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(SIDEBAR_WIDTH_MIN, SIDEBAR_HEIGHT_MIN),
            ImVec2(FLT_MAX, FLT_MAX)
        );

        if (!ImGui::Begin("Sidebar", nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::End();
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
        
        // DDE status at the top
        renderDDEStatusFrame(ddeClient, logger);

        ImGui::Spacing();

        // Page switcher buttons
        for (size_t i = 0; i < GUI_PAGES_COUNT; ++i) {
            const auto& page = GUI_PAGES[i];
            if (ImGui::Button(page.title, ImVec2(-1.0f, 0.0f))) {
                if (m_onPageSwitch) m_onPageSwitch(page.id);
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::End();
    }

    void SidebarRenderer::renderDDEStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger) {
        ImGui::BeginChild("DDE Status Frame", 
            ImVec2(DDE_STATUS_FRAME_WIDTH, DDE_STATUS_FRAME_HEIGHT), 
            ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        const char* label = "Zemax DDE Status:";
        const char* value = ddeClient && ddeClient->isConnected() ? "Connected" : "Disconnected";

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float labelWidth = ImGui::CalcTextSize(label).x;
        float valueWidth = ImGui::CalcTextSize(value).x;
        float totalWidth = labelWidth + valueWidth + 4.0f;
        float offsetX = (availableWidth - totalWidth) * 0.5f;
        if (offsetX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

        ImGui::TextUnformatted(label);
        ImGui::SameLine(0.0f, 4.0f);
        
        bool connected = ddeClient && ddeClient->isConnected();
        ImGui::PushStyleColor(ImGuiCol_Text, connected ? DDE_STATUS_COLOR_CONNECTED : DDE_STATUS_COLOR_DISCONNECTED);
        ImGui::TextUnformatted(value);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(-1.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, connected ? DDE_BUTTON_DISCONNECT_COLOR_NORMAL : DDE_BUTTON_CONNECT_COLOR_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, connected ? DDE_BUTTON_DISCONNECT_COLOR_HOVER : DDE_BUTTON_CONNECT_COLOR_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, connected ? DDE_BUTTON_DISCONNECT_COLOR_ACTIVE : DDE_BUTTON_CONNECT_COLOR_ACTIVE);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        
        if (ImGui::Button(connected ? "Disconnect from Zemax" : "Connect to Zemax", ImVec2(-1.0f, 0.0f))) {
            try {
                if (!connected) {
                    ddeClient->initiateDDE();
                } else {
                    ddeClient->terminateDDE();
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

    void SidebarRenderer::setPageSwitcher(std::function<void(gui::GuiPage)> cb) {
        m_onPageSwitch = std::move(cb);
    }
}
