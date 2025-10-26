#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "dde/dde_zemax_client.h"
#include "gui/content_pages/gui_page_local_surface_errors.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageLocalSurfaceErrors() {
        static int measureSurfaceNumber = 0;
        static int referenceSurfaceNumber = 0;
        static int numPoints = 128;

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();

        ImGui::SeparatorText("Measure surface info");
        ImGui::BeginChild("MeasureSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& measured = zemaxDDEClient->getMeasuredSurface();
            if (measured.isValid() && measured.id == measureSurfaceNumber) {
                ImGui::Spacing();
                ImGui::Text("Surface number: %d", measured.id);
                ImGui::Text("Surface type: %s", measured.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", measured.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", measured.diameter());
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearMeasuredSurface();
                }
            } else {
                ImGui::Text("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt("##measure_surf_num", &measureSurfaceNumber, 1, 10);
                measureSurfaceNumber = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, measureSurfaceNumber));
                if (ImGui::Button("Get measure surface data")) {
                    if (zemaxDDEClient) {
                        zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::MEASURED);
                        zemaxDDEClient->getSurfaceData(measureSurfaceNumber, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                        zemaxDDEClient->getSurfaceData(measureSurfaceNumber, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                    }
                }
                ImGui::TextDisabled("No data for this surface");
            }
        ImGui::EndChild();

        ImGui::SeparatorText("Reference surface info");
        ImGui::BeginChild("ReferenceSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& reference = zemaxDDEClient->getReferenceSurface();
            if (reference.isValid() && reference.id == referenceSurfaceNumber) {
                ImGui::Spacing();
                ImGui::Text("Surface number: %d", reference.id);
                ImGui::Text("Surface type: %s", reference.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", reference.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", reference.diameter());
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearReferenceSurface();
                }
            } else {
                ImGui::Text("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt("##reference_surf_num", &referenceSurfaceNumber, 1, 10);
                referenceSurfaceNumber = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, referenceSurfaceNumber));

                if (ImGui::Button("Get reference surface data")) {
                    if (zemaxDDEClient) {
                        zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::REFERENCE);
                        zemaxDDEClient->getSurfaceData(referenceSurfaceNumber, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                        zemaxDDEClient->getSurfaceData(referenceSurfaceNumber, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                    }
                }
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
