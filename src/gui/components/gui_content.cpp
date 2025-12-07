#include "gui/components/gui_content.h"

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

        gui::renderPageHeader(currentPage);

        switch (currentPage) {
            case GuiPage::OpticalSystemInfo:
                renderPageOpticalSystemInfo();
                break;
            case GuiPage::SurfaceSagAnalysis:
                renderPageSurfaceSagAnalysis();
                break;
        }

        ImGui::End();
    }
}
