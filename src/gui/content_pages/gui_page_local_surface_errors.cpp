#include "gui/gui.h"

namespace {
    std::string getSamplingTooltip() {
        return "Number of points to sample along the surface diameter (min=" +
               std::to_string(MIN_SAMPLING) + ", max=" + std::to_string(MAX_SAMPLING) +
               ").\nHigher values = smoother profile, slower calculation.";
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderPageLocalSurfaceErrors() {
        auto& state = surfaceErrorsPageState;
        auto& nominal = zemaxDDEClient->getNominalSurface();
        auto& toleranced = zemaxDDEClient->getTolerancedSurface();

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL SURFACE ERRORS");
        ImGui::Spacing();

        ImGui::SeparatorText("Toleranced surface parameters");
        ImGui::BeginChild("TolerancedSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);

        if (toleranced.isValid() && toleranced.id == state.tolerancedSurfaceIndex) {
            ImGui::Text("Optical system: %s", "null");
            ImGui::Text("Surface index: %d", toleranced.id);
            ImGui::Text("Surface type: %s", toleranced.type.c_str());
            ImGui::Text("Semi-diameter: %.3f %s", toleranced.semiDiameter, getUnitString(toleranced.units));
            ImGui::Text("Diameter: %.3f %s", toleranced.diameter(), getUnitString(toleranced.units));
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
            ImGui::Text("Sampling:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
            state.tolerancedSampling = std::max(MIN_SAMPLING, std::min(MAX_SAMPLING, state.tolerancedSampling));
            ImGui::SameLine(); 
            HelpMarker(getSamplingTooltip().c_str());

            ImGui::Text("Angle:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("##toleranced_angle", &state.tolerancedAngle, 1.0, 10.0, "%.2f");
            state.tolerancedAngle = std::clamp(state.tolerancedAngle, -360.0, 360.0);
            ImGui::SameLine(); 
            HelpMarker(getAngleTooltip().c_str());

            ImGui::Text("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
            state.tolerancedSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, state.tolerancedSurfaceIndex));

            if (ImGui::Button("Copy nominal surface settings")) {
                state.tolerancedSampling = state.nominalSampling;
                state.tolerancedAngle = state.nominalAngle;
            }

            ImGui::SameLine();

            if (ImGui::Button("Get toleranced surface data")) {
                if (isDdeInitialized()) {
                    zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::TOLERANCED);
                    zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                    zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                    calculateSurfaceProfile(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle);
                }
            }

            ImGui::Spacing();
            ImGui::TextDisabled("No data for this surface");

            ImGui::EndDisabled();
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild("NominalSurfaceContent", ImVec2(0.0f, 0.0f), ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_AutoResizeY, ImGuiChildFlags_FrameStyle);
            if (nominal.isValid() && nominal.id == state.nominalSurfaceIndex) {
                ImGui::Text("Optical system: %s", "null");
                ImGui::Text("Surface index: %d", nominal.id);
                ImGui::Text("Surface type: %s", nominal.type.c_str());
                ImGui::Text("Semi-diameter: %.3f %s", nominal.semiDiameter, getUnitString(nominal.units));
                ImGui::Text("Diameter: %.3f %s", nominal.diameter(), getUnitString(nominal.units));
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

                ImGui::Text("Sampling:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt("##nominal_sampling", &state.nominalSampling, 10, 50);
                state.nominalSampling = std::max(MIN_SAMPLING, std::min(MAX_SAMPLING, state.nominalSampling));
                ImGui::SameLine(); 
                HelpMarker(getSamplingTooltip().c_str());

                ImGui::Text("Angle:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble("##nominal_angle", &state.nominalAngle, 1.0, 10.0, "%.2f");
                state.nominalAngle = std::clamp(state.nominalAngle, -360.0, 360.0);
                ImGui::SameLine(); 
                HelpMarker(getAngleTooltip().c_str());

                ImGui::Text("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
                state.nominalSurfaceIndex = std::max(0, std::min(zemaxDDEClient->getOpticalSystemData().numSurfs, state.nominalSurfaceIndex));

                if (ImGui::Button("Copy toleranced surface settings")) {
                    state.nominalSampling = state.tolerancedSampling;
                    state.nominalAngle = state.tolerancedAngle;
                }

                ImGui::SameLine();

                if (ImGui::Button("Get nominal surface data")) {
                    if (isDdeInitialized()) {
                        zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::NOMINAL);
                        zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                        zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                        calculateSurfaceProfile(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle);
                    }
                }

                ImGui::Spacing();
                ImGui::TextDisabled("No data for this surface");

                ImGui::EndDisabled();
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
