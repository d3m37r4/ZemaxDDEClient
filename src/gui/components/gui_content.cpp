#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "dde_client.h"
#include "gui/components/gui_content.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderContent() {
        // TODO: Move to 'gui.h' as static constexpr for reuse
        float content_height = 450.0f;
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
    }
}
