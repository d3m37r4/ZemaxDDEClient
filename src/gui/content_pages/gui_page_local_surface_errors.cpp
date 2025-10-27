#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageLocalSurfaceErrors() {
        static int tolerancedSurfaceIndex = 0;
        static int nominalSurfaceIndex = 0;
        static int numPoints = 128;

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();

        ImGui::SeparatorText("Toleranced surface parameters");
        ImGui::BeginChild("TolerancedSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& toleranced = zemaxDDEClient->getTolerancedSurface();
            if (toleranced.isValid() && toleranced.id == tolerancedSurfaceIndex) {
                ImGui::Text("Optical system: %s", "null");
                ImGui::Text("Surface index: %d", toleranced.id);
                ImGui::Text("Surface type: %s", toleranced.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", toleranced.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", toleranced.diameter());
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearTolerancedSurface();
                }
            } else {
                ImGui::BeginDisabled(!isDdeInitialized());
                    ImGui::Text("Surface number:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputInt("##toleranced_surf_num", &tolerancedSurfaceIndex, 1, 10);
                    tolerancedSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, tolerancedSurfaceIndex));
                    if (ImGui::Button("Get toleranced surface data")) {
                        if (isDdeInitialized()) {
                            zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::TOLERANCED);
                            zemaxDDEClient->getSurfaceData(tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                            zemaxDDEClient->getSurfaceData(tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                            zemaxDDEClient->getSag(tolerancedSurfaceIndex, 1.0, 0.0);
                        }
                    }
                ImGui::EndDisabled();
                ImGui::TextDisabled("No data for this surface");
            }
        ImGui::EndChild();

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild("NominalSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& nominal = zemaxDDEClient->getNominalSurface();
            if (nominal.isValid() && nominal.id == nominalSurfaceIndex) {
                ImGui::Text("Optical system: %s", "null");
                ImGui::Text("Surface index: %d", nominal.id);
                ImGui::Text("Surface type: %s", nominal.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", nominal.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", nominal.diameter());
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearNominalSurface();
                }
            } else {
                ImGui::BeginDisabled(!isDdeInitialized());
                    ImGui::Text("Surface number:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputInt("##nominal_surf_num", &nominalSurfaceIndex, 1, 10);
                    nominalSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, nominalSurfaceIndex));
                    if (ImGui::Button("Get nominal surface data")) {
                        if (isDdeInitialized()) {
                            zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::NOMINAL);
                            zemaxDDEClient->getSurfaceData(nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                            zemaxDDEClient->getSurfaceData(nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                        }
                    }
                ImGui::EndDisabled();
                ImGui::TextDisabled("No data for this surface");
            }
        ImGui::EndChild();

        ImGui::SeparatorText("Settings");
        ImGui::BeginChild("SettingsContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            ImGui::Text("Number of points:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##num_points", &numPoints, 10, 50);
            numPoints = std::max(2, std::min(1024, numPoints));
        ImGui::EndChild();
    }

}
