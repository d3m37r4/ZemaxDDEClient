#include <fstream>

#include "gui/gui.h"

namespace {
    struct SagCrossSectionPair {
        std::vector<double> x_nom, y_nom;
        std::vector<double> x_tol, y_tol;
    };

    SagCrossSectionPair prepareSagCrossSectionData(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced) {
        auto [x_nom, y_nom] = gui::extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = gui::extractSagCoordinates(toleranced);
        
        return SagCrossSectionPair{
            std::move(x_nom), std::move(y_nom),
            std::move(x_tol), std::move(y_tol)
        };
    }

    std::string getSamplingTooltip() {
        return std::format(
            "Number of points to sample along the surface diameter (min={}, max={}).\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderPageSurfaceSagAnalysis() {
        auto& state = m_surfaceSagAnalysisPageState;
        auto& nominal = m_zemaxDDEClient->getNominalSurface();
        auto& toleranced = m_zemaxDDEClient->getTolerancedSurface();

        ImGui::SeparatorText("Toleranced surface parameters");
        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        if (toleranced.isValid() && toleranced.id == state.tolerancedSurfaceIndex) {
            ImGui::TextUnformatted("Optical system: null");
            ImGui::TextUnformatted(std::format("Surface index: {}", toleranced.id).c_str());
            ImGui::TextUnformatted(std::format("Surface type: {}", toleranced.type).c_str());
            ImGui::TextUnformatted(std::format("Semi-diameter: {:.3f} {}", toleranced.semiDiameter, getUnitString(toleranced.units)).c_str());
            ImGui::TextUnformatted(std::format("Diameter: {:.3f} {}", toleranced.diameter(), getUnitString(toleranced.units)).c_str());
            ImGui::Text("Surface sag data:");
            ImGui::SameLine();

            if (ImGui::Button("Text")) {
                saveSagCrossSectionToFile(toleranced);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_showTolerancedSagWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_showTolerancedSagWindow = false;
                m_zemaxDDEClient->clearTolerancedSurface();
            }
        } else {
            ImGui::BeginDisabled(!isDdeInitialized());
            ImGui::Text("Sampling:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
            state.tolerancedSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.tolerancedSampling));
            ImGui::SameLine();
            HelpMarker(getSamplingTooltip().c_str());

            ImGui::Text("Angle:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputDouble("##toleranced_angle", &state.tolerancedAngle, 1.0, 10.0, "%.2f");
            state.tolerancedAngle = std::clamp(state.tolerancedAngle, -360.0, 360.0);
            ImGui::SameLine();
            HelpMarker(getAngleTooltip().c_str());

            ImGui::Text("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
            state.tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.tolerancedSurfaceIndex));

            if (ImGui::Button("Copy nominal surface settings")) {
                state.tolerancedSampling = state.nominalSampling;
                state.tolerancedAngle = state.nominalAngle;
            }

            ImGui::SameLine();

            if (ImGui::Button("Get toleranced surface data")) {
                if (isDdeInitialized()) {
                    m_zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::TOLERANCED);
                    m_zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                    m_zemaxDDEClient->getSurfaceData(state.tolerancedSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                    calculateSagCrossSection(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle);
                }
            }

            ImGui::EndDisabled();
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );
        if (nominal.isValid() && nominal.id == state.nominalSurfaceIndex) {
            ImGui::TextUnformatted("Optical system: null");
            ImGui::TextUnformatted(std::format("Surface index: {}", nominal.id).c_str());
            ImGui::TextUnformatted(std::format("Surface type: {}", nominal.type).c_str());
            ImGui::TextUnformatted(std::format("Semi-diameter: {:.3f} {}", nominal.semiDiameter, getUnitString(nominal.units)).c_str());
            ImGui::TextUnformatted(std::format("Diameter: {:.3f} {}", nominal.diameter(), getUnitString(nominal.units)).c_str());
            ImGui::Text("Surface sag data:");
            ImGui::SameLine();

            if (ImGui::Button("Text")) {
                saveSagCrossSectionToFile(nominal);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_showNominalSagWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_showNominalSagWindow = false;
                m_zemaxDDEClient->clearNominalSurface();
            }
        } else {
            ImGui::BeginDisabled(!isDdeInitialized());

            ImGui::Text("Sampling:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##nominal_sampling", &state.nominalSampling, 10, 50);
            state.nominalSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.nominalSampling));
            ImGui::SameLine();
            HelpMarker(getSamplingTooltip().c_str());

            ImGui::Text("Angle:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputDouble("##nominal_angle", &state.nominalAngle, 1.0, 10.0, "%.2f");
            state.nominalAngle = std::clamp(state.nominalAngle, -360.0, 360.0);
            ImGui::SameLine();
            HelpMarker(getAngleTooltip().c_str());

            ImGui::Text("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
            state.nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.nominalSurfaceIndex));

            if (ImGui::Button("Copy toleranced surface settings")) {
                state.nominalSampling = state.tolerancedSampling;
                state.nominalAngle = state.tolerancedAngle;
            }

            ImGui::SameLine();

            if (ImGui::Button("Get nominal surface data")) {
                if (isDdeInitialized()) {
                    m_zemaxDDEClient->setStorageTarget(ZemaxDDE::StorageTarget::NOMINAL);
                    m_zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                    m_zemaxDDEClient->getSurfaceData(state.nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                    calculateSagCrossSection(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle);
                }
            }

            ImGui::EndDisabled();
        }
        ImGui::EndChild();
        
        if (nominal.isValid() && toleranced.isValid()) {
            ImGui::SeparatorText("Profile Comparison");

            if (ImGui::Button("Detach Comparison")) {
                m_showComparisonWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Detach Error")) {
                m_showErrorWindow = true;
            }

            ImGui::SameLine();

            // if (ImGui::Button("Export comparison to CSV")) {
            //     nfdchar_t* savePath = nullptr;
            //     nfdresult_t result = NFD_SaveDialog("csv", nullptr, &savePath);

            //     if (result == NFD_OKAY) {
            //         std::ofstream file(savePath);

            //         if (file.is_open()) {
            //             auto data = prepareSagCrossSectionData(nominal, toleranced);

            //             file << "x_nom,y_nom,x_tol,y_tol,error\n";

            //             size_t n = std::min(data.x_nom.size(), data.x_tol.size());
                        
            //             for (size_t i = 0; i < n; ++i) {
            //                 double error = data.y_tol[i] - data.y_nom[i];
            //                 file << data.x_nom[i] << "," << data.y_nom[i] << ","
            //                     << data.x_tol[i] << "," << data.y_tol[i] << ","
            //                     << error << "\n";
            //             }

            //             file.close();
            //             logger.addLog("[GUI] Comparison data saved to " + std::string(savePath));
            //         }

            //         free(savePath);
            //     }
            // }

            bool showProfiles = !m_showComparisonWindow;
            bool showErrorPlot = !m_showErrorWindow;

            if (showProfiles || showErrorPlot) {
                ImGui::BeginChild("ComparisonContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_NoTitleBar);

                auto data = prepareSagCrossSectionData(nominal, toleranced);

                if (showProfiles) {
                    if (ImPlot::BeginPlot("##Profiles", ImVec2(-1, 200))) {
                        ImPlot::SetupAxes("X (mm)", "Sag (mm)");
                        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                        ImPlot::PlotLine("Nominal", data.x_nom.data(), data.y_nom.data(), data.x_nom.size());
                        ImPlot::PlotLine("Toleranced", data.x_tol.data(), data.y_tol.data(), data.x_tol.size());
                        ImPlot::EndPlot();
                    }
                }

                ImGui::Spacing();

                if (showErrorPlot && data.x_nom.size() == data.x_tol.size()) {
                    if (ImPlot::BeginPlot("##Error", ImVec2(-1, 200))) {
                        ImPlot::SetupAxes("X (mm)", "ΔSag (mm)");
                        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);

                        std::vector<double> error;

                        for (size_t i = 0; i < data.x_nom.size(); ++i) {
                            error.push_back(data.y_nom[i] - data.y_tol[i]);
                        }

                        ImPlot::PlotLine("Error", data.x_nom.data(), error.data(), data.x_nom.size());
                        ImPlot::EndPlot();
                    }
                }

                ImGui::EndChild();
            }
        }

    }
}
