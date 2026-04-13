#include <fstream>

#include "gui/gui_impl.h"

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

        if (m_showTolerancedSagWindow) {
            auto& surface = m_zemaxDDEClient->getTolerancedSurface();
            std::string title = std::format("Toleranced Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);

            if (surface.isValid()) {
                renderSagCrossSectionWindow(title.c_str(), "Toleranced", surface, &m_showTolerancedSagWindow);
            }
        }

        if (m_showNominalSagWindow) {
            auto& surface = m_zemaxDDEClient->getNominalSurface();
            std::string title = std::format("Nominal Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);

            if (surface.isValid()) {
                renderSagCrossSectionWindow(title.c_str(), "Nominal", surface, &m_showNominalSagWindow);
            }
        }

        if (m_showComparisonWindow) {
            auto& nom = m_zemaxDDEClient->getNominalSurface();
            auto& tol = m_zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid()) {
                renderComparisonWindow(nom, tol, &m_showComparisonWindow);
            }
        }

        if (m_showErrorWindow) {
            auto& nom = m_zemaxDDEClient->getNominalSurface();
            auto& tol = m_zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid() && nom.sagDataPoints.size() == tol.sagDataPoints.size()) {
                renderErrorWindow(nom, tol, &m_showErrorWindow);
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
