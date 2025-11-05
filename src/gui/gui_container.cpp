#include "gui.h"

namespace gui {
    void GuiManager::render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderNavbar();

        // --- DockSpace: занимает всё пространство под меню ---
        const float menuHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(0, menuHeight));
        ImGui::SetNextWindowSize(ImVec2(
            ImGui::GetIO().DisplaySize.x,
            ImGui::GetIO().DisplaySize.y - menuHeight
        ));
        ImGui::Begin("MainDockSpace", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );

        // Создаём док-пространство
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        ImGui::End();


        // const float menuHeight = ImGui::GetFrameHeight();
        // const ImVec2 workPos = ImVec2(0.0f, menuHeight);
        // const ImVec2 workSize = ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - menuHeight);

        // ImGui::SetNextWindowPos(workPos);
        // ImGui::SetNextWindowSize(workSize);

        // ImGui::Begin("##MainWindow", nullptr,
        //     ImGuiWindowFlags_NoTitleBar |
        //     ImGuiWindowFlags_NoResize |
        //     ImGuiWindowFlags_NoMove |
        //     ImGuiWindowFlags_NoCollapse
        // );

        ImGui::Begin("##Sidebar");
        renderSidebar();
        ImGui::End();

        // ImGui::SameLine();
        ImGui::Begin("##Content");
        renderContent();
        ImGui::End();
        // ImGui::Spacing();
ImGui::Begin("##debug");
        renderDebugLogFrame();
ImGui::End();
        // ImGui::End();

        if (showTolerancedProfileWindow) {
            auto& surface = zemaxDDEClient->getTolerancedSurface();
            if (surface.isValid()) renderProfileWindow("Toleranced Surface Profile", "Toleranced", surface, &showTolerancedProfileWindow);
        }
        if (showNominalProfileWindow) {
            auto& surface = zemaxDDEClient->getNominalSurface();
            if (surface.isValid()) renderProfileWindow("Nominal Surface Profile", "Nominal", surface, &showNominalProfileWindow);
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
