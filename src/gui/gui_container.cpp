#include "gui.h"

namespace gui {
    void GuiManager::render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderNavbar();

        const float menuHeight = ImGui::GetFrameHeight();
        const ImVec2 workPos = ImVec2(0.0f, menuHeight);
        const ImVec2 workSize = ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - menuHeight);

        ImGui::SetNextWindowPos(workPos);
        ImGui::SetNextWindowSize(workSize);

        ImGui::Begin("##MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse
        );
        renderSidebar();
        ImGui::SameLine();
        renderContent();
        ImGui::Spacing();
        renderDebugLogFrame();
        ImGui::End();

        if (showTolerancedProfileWindow) {
            auto& surface = zemaxDDEClient->getTolerancedSurface();
            if (surface.isValid()) renderProfileWindow("Toleranced Surface Profile", "Toleranced", surface, &showTolerancedProfileWindow);
        }
        if (showNominalProfileWindow) {
            auto& surface = zemaxDDEClient->getNominalSurface();
            if (surface.isValid()) renderProfileWindow("Nominal Surface Profile", "Nominal", surface, &showNominalProfileWindow);
        }

        setPopupPosition();
        renderUpdatesPopup();
        renderAboutPopup();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
