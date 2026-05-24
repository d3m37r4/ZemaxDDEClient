#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <format>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/sag_analysis_service.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"

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
    void GuiManager::renderSurfaceSagAnalysis() {
        auto& state = m_sagService->m_surfaceSagAnalysisPageState;
        auto& nominal = m_sagService->m_nominalSurfaceData;
        auto& toleranced = m_sagService->m_tolerancedSurfaceData;

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );
        if (nominal.isValid() && nominal.id == state.nominalSurfaceIndex) {
            ImGui::TextUnformatted(std::format("Optical system: {}", nominal.fileName).c_str());
            ImGui::TextUnformatted(std::format("Surface index: {}", nominal.id).c_str());
            ImGui::TextUnformatted(std::format("Sampling: {}", nominal.sampling).c_str());
            ImGui::TextUnformatted(std::format("Angle: {}°", nominal.angle).c_str());
            ImGui::TextUnformatted(std::format("Surface type: {}", nominal.type).c_str());
            ImGui::TextUnformatted(std::format("Semi-diameter: {:.3f} {}", nominal.semiDiameter, getUnitString(nominal.units)).c_str());
            ImGui::TextUnformatted(std::format("Diameter: {:.3f} {}", nominal.diameter(), getUnitString(nominal.units)).c_str());
            ImGui::Text("Surface sag data:");
            ImGui::SameLine();

            if (ImGui::Button("Text")) {
                m_sagService->saveCrossSectionToFile(nominal);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_sagService->m_showNominalSagWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_sagService->m_showNominalSagWindow = false;
                nominal.clear();
            }
        } else {
            if (!isDDEInitialized()) {
                ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
                ImGui::Spacing();
            }

            ImGui::BeginDisabled(!isDDEInitialized());

            {
                auto fileName = m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().fileName : std::string{};
                ImGui::Text("Optical system:");
                ImGui::SameLine();
                ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::Text("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
            state.nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0, state.nominalSurfaceIndex));

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

            if (ImGui::Button("Copy toleranced surface settings")) {
                state.nominalSampling = state.tolerancedSampling;
                state.nominalAngle = state.tolerancedAngle;
            }

            ImGui::SameLine();

            if (m_sagService->getCalcState() == SagCalcState::FetchingSurfaceData ||
                m_sagService->getCalcState() == SagCalcState::FetchingSagPoints) {
                float progress = m_sagService->getTotalSteps() > 0
                    ? static_cast<float>(m_sagService->getCurrentStep()) / m_sagService->getTotalSteps()
                    : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1, 0),
                    std::format("{}/{}", m_sagService->getCurrentStep(), m_sagService->getTotalSteps()).c_str());

                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_sagService->cancelCalculation();
                }

                if (m_sagService->getSkippedPoints() > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                        "%d points skipped (timeout)", m_sagService->getSkippedPoints());
                }
            } else {
                if (ImGui::Button("Get nominal surface data")) {
                    if (isDDEInitialized()) {
                        nominal.units = m_zemaxDDEClient->getOpticalSystemData().units;
                        nominal.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                        m_sagService->onCalculationComplete = [this, &nominal]() {
                            const auto& result = m_sagService->getResult();
                            nominal.id = result.id;
                            nominal.type = result.type;
                            nominal.units = result.units;
                            nominal.semiDiameter = result.semiDiameter;
                            nominal.sampling = result.sampling;
                            nominal.angle = result.angle;
                            nominal.sagDataPoints = result.sagDataPoints;
                        };
                        m_sagService->startAsyncSagCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle);
                    }
                }
            }

            ImGui::EndDisabled();
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Toleranced surface parameters");
        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        if (toleranced.isValid() && toleranced.id == state.tolerancedSurfaceIndex) {
            ImGui::TextUnformatted(std::format("Optical system: {}", toleranced.fileName).c_str());
            ImGui::TextUnformatted(std::format("Surface index: {}", toleranced.id).c_str());
            ImGui::TextUnformatted(std::format("Sampling: {}", toleranced.sampling).c_str());
            ImGui::TextUnformatted(std::format("Angle: {}°", toleranced.angle).c_str());
            ImGui::TextUnformatted(std::format("Surface type: {}", toleranced.type).c_str());
            ImGui::TextUnformatted(std::format("Semi-diameter: {:.3f} {}", toleranced.semiDiameter, getUnitString(toleranced.units)).c_str());
            ImGui::TextUnformatted(std::format("Diameter: {:.3f} {}", toleranced.diameter(), getUnitString(toleranced.units)).c_str());
            ImGui::Text("Surface sag data:");
            ImGui::SameLine();

            if (ImGui::Button("Text")) {
                m_sagService->saveCrossSectionToFile(toleranced);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_sagService->m_showTolerancedSagWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_sagService->m_showTolerancedSagWindow = false;
                toleranced.clear();
            }
        } else {
            if (!isDDEInitialized()) {
                ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
                ImGui::Spacing();
            }

            ImGui::BeginDisabled(!isDDEInitialized());

            {
                auto fileName = m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().fileName : std::string{};
                ImGui::Text("Optical system:");
                ImGui::SameLine();
                ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::Text("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
            state.tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0, state.tolerancedSurfaceIndex));

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

            if (ImGui::Button("Copy nominal surface settings")) {
                state.tolerancedSampling = state.nominalSampling;
                state.tolerancedAngle = state.nominalAngle;
            }

            ImGui::SameLine();

            bool isCalculating = (m_sagService->getCalcState() == SagCalcState::FetchingSurfaceData ||
                                  m_sagService->getCalcState() == SagCalcState::FetchingSagPoints);

            if (isCalculating) {
                float progress = m_sagService->getTotalSteps() > 0
                    ? static_cast<float>(m_sagService->getCurrentStep()) / m_sagService->getTotalSteps()
                    : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1, 0),
                    std::format("{}/{}", m_sagService->getCurrentStep(), m_sagService->getTotalSteps()).c_str());

                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_sagService->cancelCalculation();
                }

                if (m_sagService->getSkippedPoints() > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                        "%d points skipped (timeout)", m_sagService->getSkippedPoints());
                }
            } else {
                if (ImGui::Button("Get toleranced surface data")) {
                    if (isDDEInitialized()) {
                        toleranced.units = m_zemaxDDEClient->getOpticalSystemData().units;
                        toleranced.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                        m_sagService->onCalculationComplete = [this, &toleranced]() {
                            const auto& result = m_sagService->getResult();
                            toleranced.id = result.id;
                            toleranced.type = result.type;
                            toleranced.units = result.units;
                            toleranced.semiDiameter = result.semiDiameter;
                            toleranced.sampling = result.sampling;
                            toleranced.angle = result.angle;
                            toleranced.sagDataPoints = result.sagDataPoints;
                        };
                        m_sagService->startAsyncSagCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle);
                    }
                }
            }

            ImGui::EndDisabled();
        }
        ImGui::EndChild();

        if (nominal.isValid() && toleranced.isValid()) {
            ImGui::SeparatorText("Profile Comparison");

            if (ImGui::Button("Detach Comparison")) {
                m_sagService->m_showComparisonWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Detach Error")) {
                m_sagService->m_showErrorWindow = true;
            }

            ImGui::SameLine();

            bool showProfiles = !m_sagService->m_showComparisonWindow;
            bool showErrorPlot = !m_sagService->m_showErrorWindow;

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
