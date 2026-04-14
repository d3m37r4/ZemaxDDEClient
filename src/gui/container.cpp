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

        // Sag analysis windows (delegated to service)
        if (m_sagService->m_showTolerancedSagWindow) {
            auto& surface = m_zemaxDDEClient->getTolerancedSurface();
            if (surface.isValid()) {
                std::string title = std::format("Toleranced Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);
                m_sagService->renderCrossSectionWindow(title.c_str(), "Toleranced", surface, &m_sagService->m_showTolerancedSagWindow);
            }
        }

        if (m_sagService->m_showNominalSagWindow) {
            auto& surface = m_zemaxDDEClient->getNominalSurface();
            if (surface.isValid()) {
                std::string title = std::format("Nominal Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);
                m_sagService->renderCrossSectionWindow(title.c_str(), "Nominal", surface, &m_sagService->m_showNominalSagWindow);
            }
        }

        if (m_sagService->m_showComparisonWindow) {
            auto& nom = m_zemaxDDEClient->getNominalSurface();
            auto& tol = m_zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid()) {
                m_sagService->renderComparisonWindow(nom, tol, &m_sagService->m_showComparisonWindow);
            }
        }

        if (m_sagService->m_showErrorWindow) {
            auto& nom = m_zemaxDDEClient->getNominalSurface();
            auto& tol = m_zemaxDDEClient->getTolerancedSurface();
            if (nom.isValid() && tol.isValid() && nom.sagDataPoints.size() == tol.sagDataPoints.size()) {
                m_sagService->renderErrorWindow(nom, tol, &m_sagService->m_showErrorWindow);
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
