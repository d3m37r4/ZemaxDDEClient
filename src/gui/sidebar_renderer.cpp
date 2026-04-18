#include "gui/sidebar_renderer.h"
#include "gui/gui.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void SidebarRenderer::renderSidebar(ZemaxDDE::ZemaxDDEClient* ddeClient) {
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

        // DDE status at the top
        renderDDEStatusFrame(ddeClient);

        ImGui::Spacing();

        // Page switcher buttons
        for (size_t i = 0; i < GUI_PAGES_COUNT; ++i) {
            const auto& page = GUI_PAGES[i];
            if (ImGui::Button(page.title, ImVec2(-1.0f, 0.0f))) {
                if (m_onPageSwitch) m_onPageSwitch(page.id);
            }
        }

        ImGui::End();
    }

    void SidebarRenderer::renderDDEStatusFrame(ZemaxDDE::ZemaxDDEClient* ddeClient) {
        ImGui::SeparatorText("DDE Status");
        
        if (!ddeClient) {
            ImGui::TextUnformatted("DDE: Not initialized");
            return;
        }

        bool connected = ddeClient->isConnected();
        
        // Status indicator
        ImVec4 statusColor = connected ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        ImGui::TextColored(statusColor, connected ? "● Connected" : "○ Disconnected");
        
        if (connected) {
            ImGui::TextUnformatted("Server: ZEMAX");
        } else {
            ImGui::TextUnformatted("Click 'Connect' to connect");
        }
    }

    void SidebarRenderer::setPageSwitcher(std::function<void(gui::GuiPage)> cb) {
        m_onPageSwitch = std::move(cb);
    }
}
