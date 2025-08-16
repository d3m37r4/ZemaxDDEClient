#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "dde/dde_zemax_handler.h"
#include "gui/content_pages/gui_page_local_surface_errors.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageLocalSurfaceErrors() {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();
        ImGui::Text("Content of page 2");
        ImGui::InputInt("Surface Number", &surface_number);
        if (surface_number < 0) surface_number = 0;

        if (ImGui::Button("Get surface info")) {
            // ZemaxDDE::getSurfaceInfo(hwndDDE, surface_number);
            ImGui::Text("Surface %d", surface_number);
            // ImGui::Text("Surface Type: %d", ZemaxDDE::surfaceInfo.surfaceType);
            // ImGui::Text("Surface Name: %s", ZemaxDDE::surfaceInfo.surfaceName); 
        }
    }
}
