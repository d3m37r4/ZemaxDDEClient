#include "content_router.h"
#include "gui.h"
#include "gui/gui.h"
#include "lib/imgui/imgui.h"
#include "gui/utils.h"

namespace gui {
    void ContentRouter::renderContent(GuiPage currentPage, GuiManager* gui) {
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(CONTENT_WIDTH_MIN, CONTENT_HEIGHT_MIN),
            ImVec2(FLT_MAX, FLT_MAX)
        );

        if (!ImGui::Begin("Workspace", nullptr)) {
            ImGui::End();
            return;
        }

        // Page header
        renderPageHeader(currentPage);

        // Render actual page content
        switch (currentPage) {
            case GuiPage::OpticalSystemInfo:
                gui->renderPageOpticalSystemInfo();
                break;
            case GuiPage::SurfaceSagAnalysis:
                gui->renderPageSurfaceSagAnalysis();
                break;
        }

        ImGui::End();
    }
}
