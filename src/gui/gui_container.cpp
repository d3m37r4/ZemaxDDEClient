#include <fstream>

#include "gui.h"

namespace gui {
    void GuiManager::render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderNavbar();

        const float navbarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(0.0f, navbarHeight));
        ImGui::SetNextWindowSize(ImVec2(
            ImGui::GetIO().DisplaySize.x,
            ImGui::GetIO().DisplaySize.y - navbarHeight
        ));
        ImGui::Begin("MainDockSpace", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );
        ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();

        renderSidebar();
        renderContent();
        renderDebugLog();

        if (showTolerancedSagWindow) {
            auto& surface = zemaxDDEClient->getTolerancedSurface();
            std::string title = std::format("Toleranced Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);

            if (surface.isValid()) {
                renderSagCrossSectionWindow(title.c_str(), "Toleranced", surface, &showTolerancedSagWindow);
            }
        }

        if (showNominalSagWindow) {
            auto& surface = zemaxDDEClient->getNominalSurface();
            std::string title = std::format("Nominal Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);

            if (surface.isValid()) {
                renderSagCrossSectionWindow(title.c_str(), "Nominal", surface, &showNominalSagWindow);
            }
        }

        if (showComparisonWindow) {
            auto& nom = zemaxDDEClient->getNominalSurface();
            auto& tol = zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid()) {
                renderComparisonWindow(nom, tol, &showComparisonWindow);
            }
        }

        if (showErrorWindow) {
            auto& nom = zemaxDDEClient->getNominalSurface();
            auto& tol = zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid() && nom.sagDataPoints.size() == tol.sagDataPoints.size()) {
                renderErrorWindow(nom, tol, &showErrorWindow);
            }
        }
        
        setPopupPosition();
        renderUpdatesPopup();
        renderAboutPopup();

        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
