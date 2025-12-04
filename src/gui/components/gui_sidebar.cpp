#include "gui/components/gui_sidebar.h"

#include "gui/gui.h"

namespace gui {
    void GuiManager::renderSidebar() {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(SIDEBAR_WIDTH_MIN, SIDEBAR_HEIGHT_MIN),      // min_size
            ImVec2(FLT_MAX, FLT_MAX)                            // max_size
        );

        if (!ImGui::Begin("Sidebar", nullptr,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::End();
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
        renderDDEStatusFrame();
        ImGui::Spacing();
        if (ImGui::Button("Optical system information", ImVec2(-1.0f, 0.0f))) selectedMenuItem = 0;
        if (ImGui::Button("Local surface errors", ImVec2(-1.0f, 0.0f))) selectedMenuItem = 1;
        ImGui::PopStyleVar(2);
        ImGui::End();
    }
}
