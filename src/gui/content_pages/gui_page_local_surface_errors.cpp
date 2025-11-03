#include "gui/gui.h"

#include <filesystem>
#include <fstream>

namespace gui {
    void GuiManager::renderProfileWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag) {
        if (!openFlag || !*openFlag) return;
        if (surface.sagDataPoints.empty()) return;

        // Размер по умолчанию: 600x400
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(title, openFlag, ImGuiWindowFlags_None)) {
            ImGui::End();
            return;
        }

        // Подготовка данных
        std::vector<double> x_vals, y_vals;
        for (const auto& point : surface.sagDataPoints) {
            x_vals.push_back(point.x);
            y_vals.push_back(point.sag);
        }

        if (ImPlot::BeginPlot(label, ImVec2(-1, -1))) {
            ImPlot::SetupAxes("X (mm)", "Sag (mm)");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine(label, x_vals.data(), y_vals.data(), x_vals.size());
            ImPlot::EndPlot();
        }

        ImGui::End();
    }

    void GuiManager::calculateSurfaceProfile(int surfaceNumber, int sampling) {
        auto& targetSurface = [&]() -> const ZemaxDDE::SurfaceData& {
            auto target = zemaxDDEClient->getStorageTarget();
            assert(target == ZemaxDDE::StorageTarget::NOMINAL || target == ZemaxDDE::StorageTarget::TOLERANCED);
            switch (target) {
                case ZemaxDDE::StorageTarget::NOMINAL: return zemaxDDEClient->getNominalSurface();
                case ZemaxDDE::StorageTarget::TOLERANCED: return zemaxDDEClient->getTolerancedSurface();
            }
        }();

        if (!targetSurface.isValid() || targetSurface.id != surfaceNumber) {
            logger.addLog("[GUI] No valid surface data for surface " + std::to_string(surfaceNumber));
            return;
        }

        double semiDiameter = targetSurface.semiDiameter;
        double step = (2.0 * semiDiameter) / (sampling - 1);

        for (int i = 0; i < sampling; ++i) {
            double x = -semiDiameter + i * step;
            double y = 0.0; // meridional profile

            zemaxDDEClient->getSag(surfaceNumber, x, y);
        }
    }

    void GuiManager::saveSagProfileToFile(const ZemaxDDE::SurfaceData& surface) {
        if (surface.sagDataPoints.empty()) {
            logger.addLog("[GUI] No sag profile data to save");
            return;
        }

        auto tempDir = std::filesystem::temp_directory_path();
        auto tempPath = tempDir / "ZemaxDDE_SagProfile.txt";

        std::ofstream file(tempPath);
        if (!file.is_open()) {
            logger.addLog("[GUI] Failed to open file for writing: " + tempPath.string());
            return;
        }

        file << "Surface 1.\n";
        file << "Coordinate units are Millimeters.\n";
        file << "Units are Millimeters.\n";
        file << "Width = 2, Decenter x = 0, y = 0 Millimeters.\n";
        file << "Cross section is oriented at an angle of 0 degrees.\n\n";

        file << std::setw(15) << "X-Coordinate"
            << std::setw(15) << "Y-Coordinate"
            << std::setw(15) << "Sag"
            << "\n";

        for (const auto& point : surface.sagDataPoints) {
            file << std::scientific << std::setprecision(6)
                << std::setw(15) << point.x
                << std::setw(15) << point.y
                << std::setw(15) << point.sag
                << "\n";
        }

        file.close();
        logger.addLog("[GUI] Sag profile saved to " + tempPath.string());

        ShellExecuteW(nullptr, L"open", tempPath.c_str(), nullptr, nullptr, SW_SHOW);
    }

    void GuiManager::renderPageLocalSurfaceErrors() {
        auto& state = localState; // LocalSurfaceErrorState

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();

        ImGui::SeparatorText("Settings");
        ImGui::BeginChild("SettingsContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            ImGui::Text("Sampling:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##sampling", &state.sampling, 10, 50);
            state.sampling = std::max(MIN_SAMPLING, std::min(MAX_SAMPLING, state.sampling));
            ImGui::SameLine(); 
            std::string tooltipSampling = "Number of points to sample along the surface diameter (min=" + 
                                  std::to_string(MIN_SAMPLING) + ", max=" + 
                                  std::to_string(MAX_SAMPLING) + 
                                  ").\nHigher values = smoother profile, slower calculation.";
            HelpMarker(tooltipSampling.c_str());

            ImGui::Text("Number of cross-sections:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##num_cross_sections", &state.numCrossSections, 1, 10);
            state.numCrossSections = std::max(1, state.numCrossSections);
            // state.numCrossSections = std::max(1, std::min(16, state.numCrossSections));
            ImGui::SameLine(); HelpMarker("Number of angular sections to analyze (e.g., 0°, 45°, 90°...)");
            if (state.numCrossSections > 180) {
                ImGui::TextDisabled("Warning: large number of sections may slow down performance");
            }
        ImGui::EndChild();

        ImGui::SeparatorText("Toleranced surface parameters");
        ImGui::BeginChild("TolerancedSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& toleranced = zemaxDDEClient->getTolerancedSurface();
            if (toleranced.isValid() && toleranced.id == state.tolerancedSurfaceIndex) {
                ImGui::Text("Optical system: %s", "null");
                ImGui::Text("Surface index: %d", toleranced.id);
                ImGui::Text("Surface type: %s", toleranced.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", toleranced.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", toleranced.diameter());
                ImGui::Text("Surface sag data:");
                ImGui::SameLine();
                if (ImGui::Button("Text")) {
                    saveSagProfileToFile(toleranced);
                }
                ImGui::SameLine();
                if (ImGui::Button("Show profile graphic")) {
                    state.showTolerancedProfileWindow = true;
                }
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearTolerancedSurface();
                }
                if (state.showTolerancedProfileWindow) {
                    renderProfileWindow("Toleranced Surface Profile", "Toleranced", toleranced, &state.showTolerancedProfileWindow);
                }
            } else {
                ImGui::BeginDisabled(!isDdeInitialized());
                    ImGui::Text("Surface number:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
                    state.tolerancedSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, state.tolerancedSurfaceIndex));
                    if (ImGui::Button("Get toleranced surface data")) {
                        if (isDdeInitialized()) {
                            zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::TOLERANCED);
                            zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                            zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                            calculateSurfaceProfile(state.tolerancedSurfaceIndex, state.sampling);
                        }
                    }
                ImGui::EndDisabled();
                ImGui::TextDisabled("No data for this surface");
            }
        ImGui::EndChild();

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild("NominalSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            auto& nominal = zemaxDDEClient->getNominalSurface();
            if (nominal.isValid() && nominal.id == state.nominalSurfaceIndex) {
                ImGui::Text("Optical system: %s", "null");
                ImGui::Text("Surface index: %d", nominal.id);
                ImGui::Text("Surface type: %s", nominal.type.c_str());
                ImGui::Text("Semi-diameter: %.3f mm", nominal.semiDiameter);
                ImGui::Text("Diameter: %.3f mm", nominal.diameter());
                ImGui::Text("Surface sag data:");
                ImGui::SameLine();
                if (ImGui::Button("Text")) {
                    saveSagProfileToFile(nominal);
                }
                ImGui::SameLine();
                if (ImGui::Button("Show profile graphic")) {
                    state.showNominalProfileWindow = true;
                }
                if (ImGui::Button("Clear data")) {
                    zemaxDDEClient->clearNominalSurface();
                }
                if (state.showNominalProfileWindow) {
                    renderProfileWindow("Nominal Surface Profile", "Nominal", nominal, &state.showNominalProfileWindow);
                }
            } else {
                ImGui::BeginDisabled(!isDdeInitialized());
                    ImGui::Text("Surface number:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
                    state.nominalSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, state.nominalSurfaceIndex));
                    if (ImGui::Button("Get nominal surface data")) {
                        if (isDdeInitialized()) {
                            zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::NOMINAL);
                            zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                            zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                            calculateSurfaceProfile(state.nominalSurfaceIndex, state.sampling);
                        }
                    }
                ImGui::EndDisabled();
                ImGui::TextDisabled("No data for this surface");
            }
        ImGui::EndChild();
    }
}
