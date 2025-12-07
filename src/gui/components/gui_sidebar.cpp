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

        for (size_t i = 0; i < GUI_PAGES_COUNT; ++i) {
            const auto& page = GUI_PAGES[i];
            if (ImGui::Button(page.title, ImVec2(-1.0f, 0.0f))) {
                currentPage = page.id;
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::End();
    }
}
