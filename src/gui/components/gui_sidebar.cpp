#include "lib/imgui/imgui.h"
#include "gui/components/gui_sidebar.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderSidebar() {
        ImGui::BeginChild("Sidebar", ImVec2(SIDEBAR_WIDTH, SIDEBAR_HEIGHT), 
        ImGuiChildFlags_AutoResizeY, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

        renderDDEStatusFrame();

        ImGui::Spacing();

        if (ImGui::Button("Optical system information", ImVec2(-1.0f, 0.0f))) selectedMenuItem = 0;
        if (ImGui::Button("Local surface errors", ImVec2(-1.0f, 0.0f))) selectedMenuItem = 1;

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }
}
