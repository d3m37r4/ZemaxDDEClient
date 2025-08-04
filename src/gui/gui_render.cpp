#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "dde_client.h"
#include "gui.h"

namespace gui {
    void GuiManager::render() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMenuBar();

        ImVec2 window_pos = ImVec2(0, ImGui::GetFrameHeight());
        float total_available_height = ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight();
        // TODO: Move to 'gui.h' as static constexpr for reuse
        float sidebar_width = 200.0f;
        float sidebar_height = 250.0f;
        float content_height = 450.0f;

        ImGui::SetNextWindowPos(window_pos);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, total_available_height));

        ImGui::Begin("Main Content", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, sidebar_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        renderDDEStatusFrame();

        ImGui::Spacing();

        if (ImGui::Button("Optical system information", ImVec2(-1, 0))) selectedMenuItem = 0;
        if (ImGui::Button("Local error analysis\nfor aspherical surface", ImVec2(-1, 0))) selectedMenuItem = 1;

        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Content", ImVec2(0, content_height), true);
        switch (selectedMenuItem) {
            case 0: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
                ImGui::PopStyleVar();
                ImGui::Spacing();

                ImGui::InputInt("Surface Number", &surface_number);
                if (surface_number < 1) surface_number = 1;

                if (ImGui::Button("Get Radius")) {
                    try {
                        if (dde_initialized) {
                            if (surface_number <= 0) {
                                setErrorMsg("Invalid surface number");
                                return;
                            }

                            logger.addLog("Calling getSurfaceRadius with hwndDDE: " + std::to_string((uintptr_t)hwndDDE));

                            float radius = ZemaxDDE::getSurfaceRadius(hwndDDE, surface_number);
                            setRadius(radius);
                            logger.addLog("Radius retrieved: " + std::to_string(radius));
                        } else {
                            setErrorMsg("DDE not initialized");
                        }
                    } catch (const std::runtime_error& e) {
                        setErrorMsg(e.what());
                        logger.addLog((std::string("Error: ") + e.what()).c_str());
                    }
                }

                ImGui::Text("Radius of Surface %d: %.4f", surface_number, radius);
                if (errorMsg[0]) ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", errorMsg);

                break;
            }
            case 1: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL ERROR ANALYSIS FOR ASPHERICAL SURFACE");
                ImGui::PopStyleVar();
                ImGui::Spacing();
                ImGui::Text("text 2");

                break;
            }
        }
        ImGui::EndChild();

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
