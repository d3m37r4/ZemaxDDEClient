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

        ImGui::BeginChild("DDE Status Frame", ImVec2(sidebar_width, 60), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("DDE Status:");
        ImGui::SameLine();

        char status_text[32];
        strncpy_s(status_text, dde_initialized ? "Initialized" : "Not Initialized", sizeof(status_text));
        ImGui::PushStyleColor(ImGuiCol_Text, dde_initialized ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##DDEStatus", status_text, sizeof(status_text), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        float button_height = ImGui::GetFrameHeight();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.0f));
        if (ImGui::Button(dde_initialized ? "Close DDE" : "Init DDE", ImVec2(-1, button_height))) {
            try {
                if (!dde_initialized) {
                    ZemaxDDE::initiateDDE(hwndDDE);
                    dde_initialized = true;
                    logger.addLog("DDE connection established successfully");
                } else {
                    ZemaxDDE::terminateDDE();
                    dde_initialized = false;
                    logger.addLog("DDE connection terminated");
                }
            } catch (const std::runtime_error& e) {
                logger.addLog((std::string("DDE Error: ") + e.what()).c_str());
                setErrorMsg(e.what());
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
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
