#include "gui/gui.h"

namespace gui {
    void GuiManager::renderContent() {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(CONTENT_WIDTH_MIN, CONTENT_HEIGHT_MIN),      // min_size
            ImVec2(FLT_MAX, FLT_MAX)                            // max_size
        );

        if (!ImGui::Begin("Workspace", nullptr)) {
            ImGui::End();
            return;
        }

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

        ImGui::End();
    }
}
