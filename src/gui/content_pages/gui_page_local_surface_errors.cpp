#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "dde/dde_zemax_client.h"
#include "gui/content_pages/gui_page_local_surface_errors.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageLocalSurfaceErrors() {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();
        ImGui::Text("Content of page 2");
    }
}
