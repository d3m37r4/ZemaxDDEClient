#include "lib/imgui/imgui.h"
#include "gui/components/gui_content.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderContent() {
        float content_height = 450.0f;      // TODO: Move to 'gui.h' as static constexpr for reuse
        ImGui::BeginChild("Content", ImVec2(0, content_height), ImGuiChildFlags_Borders);
        switch (selectedMenuItem) {
            case 0: {
                renderPageOpticalSystemInfo();
                break;
            }
            case 1: {
                renderPageLocalSurfaceErrors();
                break;
            }
        }
        ImGui::EndChild();
    }
}
