#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "gui.h"

namespace gui {
    void GuiManager::render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMenuBar();

        ImVec2 window_pos = ImVec2(0, ImGui::GetFrameHeight());
        float total_available_height = ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(window_pos);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, total_available_height));

        ImGui::Begin("Layout", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        renderSidebar();
        ImGui::SameLine();
        renderContent();
        ImGui::Spacing();
        renderDebugLogFrame();
        ImGui::End();

        setPopupPosition();
        renderUpdatesPopup();
        renderAboutPopup();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
