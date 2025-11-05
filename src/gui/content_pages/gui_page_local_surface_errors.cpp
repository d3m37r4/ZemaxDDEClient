#include "gui/gui.h"

#include <filesystem>
#include <fstream>

namespace gui {
    void GuiManager::renderProfileWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag) {
        if (!openFlag || !*openFlag) return;
        if (surface.sagDataPoints.empty()) return;
        
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(title, openFlag)) {
            ImGui::End();
            return;
        }

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

    void GuiManager::renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag) {
        if (!openFlag || !*openFlag) return;

        if (ImGui::Begin("Profile Comparison", openFlag)) {
            std::vector<double> x_nom, y_nom, x_tol, y_tol;
            for (const auto& p : nominal.sagDataPoints) {
                x_nom.push_back(p.x);
                y_nom.push_back(p.sag);
            }
            for (const auto& p : toleranced.sagDataPoints) {
                x_tol.push_back(p.x);
                y_tol.push_back(p.sag);
            }

            if (ImPlot::BeginPlot("##DetachedProfiles", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("X (mm)", "Sag (mm)");
                ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                ImPlot::PlotLine("Nominal", x_nom.data(), y_nom.data(), x_nom.size());
                ImPlot::PlotLine("Toleranced", x_tol.data(), y_tol.data(), x_tol.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
    }

    void GuiManager::renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag) {
        if (!openFlag || !*openFlag) return;
        if (nominal.sagDataPoints.size() != toleranced.sagDataPoints.size()) return;

        if (ImGui::Begin("Sag Error (Toleranced - Nominal)", openFlag)) {
            std::vector<double> x, y;
            for (size_t i = 0; i < nominal.sagDataPoints.size(); ++i) {
                x.push_back(nominal.sagDataPoints[i].x);
                y.push_back(nominal.sagDataPoints[i].sag - toleranced.sagDataPoints[i].sag);
            }

            if (ImPlot::BeginPlot("##DetachedError", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("X (mm)", "ΔSag (mm)");
                ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                ImPlot::PlotLine("Error", x.data(), y.data(), x.size());
                ImPlot::EndPlot();
            }
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
                default: throw std::runtime_error("Unexpected storage target in calculateSurfaceProfile");
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
        auto& nominal = zemaxDDEClient->getNominalSurface();
        auto& toleranced = zemaxDDEClient->getTolerancedSurface();

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
                    showTolerancedProfileWindow = true;
                }
                if (ImGui::Button("Clear data")) {
                    showTolerancedProfileWindow = false;
                    zemaxDDEClient->clearTolerancedSurface();
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
                    showNominalProfileWindow = true;
                }
                if (ImGui::Button("Clear data")) {
                    showNominalProfileWindow = false;
                    zemaxDDEClient->clearNominalSurface();
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
        
        if (nominal.isValid() && toleranced.isValid()) {
            ImGui::SeparatorText("Profile Comparison");

            if (ImGui::Button("Detach Comparison")) {
                showComparisonWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Detach Error")) {
                showErrorWindow = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Export comparison to CSV")) {
                nfdchar_t* savePath = nullptr;
                nfdresult_t result = NFD_SaveDialog("csv", nullptr, &savePath);
                if (result == NFD_OKAY) {
                    std::ofstream file(savePath);
                    if (file.is_open()) {
                        std::vector<double> x_nom, y_nom, x_tol, y_tol;
                        for (const auto& p : nominal.sagDataPoints) {
                            x_nom.push_back(p.x);
                            y_nom.push_back(p.sag);
                        }
                        for (const auto& p : toleranced.sagDataPoints) {
                            x_tol.push_back(p.x);
                            y_tol.push_back(p.sag);
                        }
                        file << "x_nom,y_nom,x_tol,y_tol,error\n";
                        size_t n = std::min(x_nom.size(), x_tol.size());
                        for (size_t i = 0; i < n; ++i) {
                            double error = y_tol[i] - y_nom[i];
                            file << x_nom[i] << "," << y_nom[i] << ","
                                << x_tol[i] << "," << y_tol[i] << ","
                                << error << "\n";
                        }
                        file.close();
                        logger.addLog("[GUI] Comparison data saved to " + std::string(savePath));
                    }
                    free(savePath);
                }
            }

            bool showProfiles = !showComparisonWindow;
            bool showErrorPlot = !showErrorWindow;

            if (showProfiles || showErrorPlot) {
                ImGui::BeginChild("ComparisonContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);

                std::vector<double> x_nom, y_nom, x_tol, y_tol;
                for (const auto& p : nominal.sagDataPoints) {
                    x_nom.push_back(p.x);
                    y_nom.push_back(p.sag);
                }
                for (const auto& p : toleranced.sagDataPoints) {
                    x_tol.push_back(p.x);
                    y_tol.push_back(p.sag);
                }

                if (showProfiles) {
                    if (ImPlot::BeginPlot("##Profiles", ImVec2(-1, 200))) {
                        ImPlot::SetupAxes("X (mm)", "Sag (mm)");
                        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                        ImPlot::PlotLine("Nominal", x_nom.data(), y_nom.data(), x_nom.size());
                        ImPlot::PlotLine("Toleranced", x_tol.data(), y_tol.data(), x_tol.size());
                        ImPlot::EndPlot();
                    }
                    ImGui::Spacing();
                }

                if (showErrorPlot && x_nom.size() == x_tol.size()) {
                    if (ImPlot::BeginPlot("##Error", ImVec2(-1, 200))) {
                        ImPlot::SetupAxes("X (mm)", "ΔSag (mm)");
                        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                        std::vector<double> error;
                        for (size_t i = 0; i < x_nom.size(); ++i) {
                            error.push_back(y_nom[i] - y_tol[i]);
                        }
                        ImPlot::PlotLine("Error", x_nom.data(), error.data(), x_nom.size());
                        ImPlot::EndPlot();
                    }
                }

                ImGui::EndChild();
            }
        }

    }
}
