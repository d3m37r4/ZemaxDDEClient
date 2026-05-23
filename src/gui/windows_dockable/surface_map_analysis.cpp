#include <algorithm>
#include <cmath>
#include <format>
#include <vector>
#include <numbers>

#include "gui/gui.h"
#include "gui/constants.h"
#include "logger/logger.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;

    std::string getSamplingTooltip() {
        return std::format(
            "Number of points to sample along the surface diameter (min={}, max={}).\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
    }

    std::string getAngleStepTooltip() {
        return "Angular step between points on each ring in degrees.\n"
               "Full circle: 0° to 360°.\n"
               "Smaller step = more detailed surface, slower calculation.";
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderSurfaceMapAnalysis() {
        if (!m_zemaxDDEClient) return;
        auto& state = m_sagMapService->m_state;

        ImGui::SeparatorText("Nominal surface parameters");

        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        ImGui::BeginDisabled(!isDDEInitialized());

        ImGui::Text("Surface number:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
        state.nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.nominalSurfaceIndex));

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

        bool isSagCalculating = (m_sagService->getCalcState() == SagCalcState::FetchingSurfaceData ||
                                 m_sagService->getCalcState() == SagCalcState::FetchingSagPoints);

        if (isSagCalculating) {
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
                    auto* nominal = m_zemaxDDEClient->getNominalSurface();
                    nominal->units = m_zemaxDDEClient->getOpticalSystemData().units;
                    nominal->fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                    m_sagService->onCalculationComplete = [this]() {
                        const auto& result = m_sagService->getResult();
                        auto* nominal = m_zemaxDDEClient->getNominalSurface();
                        nominal->semiDiameter = result.semiDiameter;
                        nominal->sampling = result.sampling;
                        nominal->angle = result.angle;
                        nominal->sagDataPoints = result.sagDataPoints;
                        m_logger.addLog("[SagMap] Nominal reference set for surface map analysis");
                    };
                    m_sagService->startAsyncSagCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle);
                }
            }
        }

        ImGui::EndDisabled();

        ImGui::EndChild();

        ImGui::SeparatorText("Toleranced surface parameters");

        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        ImGui::BeginDisabled(!isDDEInitialized());

        ImGui::Text("Surface number:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
        state.tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.tolerancedSurfaceIndex));

        ImGui::Text("Sampling:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
        state.tolerancedSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.tolerancedSampling));
        ImGui::SameLine();
        HelpMarker(getSamplingTooltip().c_str());

        ImGui::Text("Angle step:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputDouble("##toleranced_angle_step", &state.tolerancedAngleStep, 0.1, 1.0, "%.2f");
        state.tolerancedAngleStep = std::clamp(state.tolerancedAngleStep, 0.01, 360.0);
        ImGui::SameLine();
        HelpMarker(getAngleStepTooltip().c_str());

        ImGui::EndDisabled();

        ImGui::EndChild();

        ImGui::SeparatorText("Analysis");

        bool isMapCalculating = (m_sagMapService->getMapState() == SagMapState::FetchingSurfaceData ||
                                 m_sagMapService->getMapState() == SagMapState::FetchingSagPoints);

        if (isMapCalculating) {
            int total = m_sagMapService->getTotalSteps();
            int current = m_sagMapService->getCurrentStep();
            float progress = total > 0 ? static_cast<float>(current) / total : 0.0f;
            ImGui::ProgressBar(progress, ImVec2(-1, 0),
                std::format("{}/{}", current, total).c_str());

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                m_sagMapService->cancelCalculation();
            }

            if (m_sagMapService->getSkippedPoints() > 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                    "%d points skipped (timeout)", m_sagMapService->getSkippedPoints());
            }
        } else {
            ImGui::BeginDisabled(!isDDEInitialized());
            if (ImGui::Button("Calculate Surface Map")) {
                m_sagMapService->startAsyncMapCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngleStep);
            }
            ImGui::EndDisabled();
        }

        // TODO: Re-enable Max-PV analysis
        /*
        ImGui::SameLine();

        ImGui::BeginDisabled(!isDDEInitialized() || !hasData || !hasNominal);
        if (ImGui::Button("Find Max-PV Section")) {
            auto result = m_sagMapService->findMaxPVSection();
            if (result.has_value()) {
                m_logger.addLog(std::format("[SagMap] Max-PV: Angle={:.2f}°, Peak={:.6f}, Valley={:.6f}, PV={:.6f}",
                    result->angle, result->peak, result->valley, result->pv));
            }
        }
        ImGui::EndDisabled();
        */

        bool hasData = m_sagMapService->hasData();

        if (hasData) {
            ImGui::SeparatorText("Results");
            ImGui::Text(std::format("Rings calculated: {}", m_sagMapService->getSections().size()).c_str());

            const auto& sections = m_sagMapService->getSections();
            int numRadii = static_cast<int>(sections.size());
            if (numRadii > 0) {
                int numAngles = static_cast<int>(sections[0].sagDataPoints.size());
                double semiDiameter = sections[0].semiDiameter;
                double angleStepDeg = m_sagMapService->m_state.tolerancedAngleStep;
                double radiusStep = semiDiameter / (numRadii - 1);

                std::vector<float> X(numRadii * numAngles);
                std::vector<float> Y(numRadii * numAngles);
                std::vector<float> Z(numRadii * numAngles);

                float zMin = 0, zMax = 0;
                bool first = true;

                for (int i = 0; i < numRadii; ++i) {
                    double r = i * radiusStep;
                    for (int j = 0; j < numAngles; ++j) {
                        double angle = j * angleStepDeg * DEG_TO_RAD;
                        X[i * numAngles + j] = static_cast<float>(r * std::cos(angle));
                        Y[i * numAngles + j] = static_cast<float>(r * std::sin(angle));

                        const auto& pt = sections[i].sagDataPoints[j];
                        Z[i * numAngles + j] = static_cast<float>(pt.sag);

                        if (first) {
                            zMin = pt.sag;
                            zMax = pt.sag;
                            first = false;
                        } else {
                            zMin = std::min(zMin, static_cast<float>(pt.sag));
                            zMax = std::max(zMax, static_cast<float>(pt.sag));
                        }
                    }
                }

                ImGui::Text("Toleranced Surface (absolute sag):");
                if (ImPlot3D::BeginPlot("##Surface3D", ImVec2(-1, 400))) {
                    ImPlot3D::SetupAxes("X (mm)", "Y (mm)", "Z (mm)");
                    ImPlot3D::PlotSurface("Lens Surface", X.data(), Y.data(), Z.data(),
                                          numRadii, numAngles, zMin, zMax);
                    ImPlot3D::EndPlot();
                }

                // TODO: Re-enable deviation heatmap
                /*
                if (hasNominal) {
                    const auto& nominal = m_sagMapService->getNominalReference();
                    if (nominal.sagDataPoints.size() == numRadialPoints) {
                        std::vector<double> deviationValues(numAngles * numRadialPoints);
                        double devMin = 0, devMax = 0;
                        bool first = true;

                        for (int i = 0; i < numAngles; ++i) {
                            for (int j = 0; j < numRadialPoints; ++j) {
                                double deviation = sections[i].sagDataPoints[j].sag - nominal.sagDataPoints[j].sag;
                                deviationValues[i * numRadialPoints + j] = deviation;
                                if (first) {
                                    devMin = deviation;
                                    devMax = deviation;
                                    first = false;
                                } else {
                                    devMin = std::min(devMin, deviation);
                                    devMax = std::max(devMax, deviation);
                                }
                            }
                        }

                        double absMax = std::max(std::abs(devMin), std::abs(devMax));

                        ImGui::Spacing();
                        ImGui::Text("Deviation (Toleranced - Nominal):");
                        if (ImPlot::BeginPlot("##DeviationMap", ImVec2(-1, 200))) {
                            ImPlot::SetupAxes("Angle (deg)", "Radial Position");
                            ImPlot::PlotHeatmap("##DeviationHeatmap", deviationValues.data(), numAngles, numRadialPoints,
                                -absMax, absMax, nullptr,
                                ImPlotPoint(0, 0), ImPlotPoint(180, numRadialPoints));
                            ImPlot::EndPlot();
                        }
                    }
                }
                */

                // TODO: Re-enable Max-PV analysis
                /*
                auto maxPV = m_sagMapService->findMaxPVSection();
                if (maxPV.has_value()) {
                    ImGui::SeparatorText("Max-PV Result");
                    auto& pv = maxPV.value();
                    ImGui::TextUnformatted(std::format("Max-PV Section at {:.2f}°", pv.angle).c_str());
                    ImGui::TextUnformatted(std::format("Peak: {:.6f} mm", pv.peak).c_str());
                    ImGui::TextUnformatted(std::format("Valley: {:.6f} mm", pv.valley).c_str());
                    ImGui::TextUnformatted(std::format("PV: {:.6f} mm", pv.pv).c_str());
                }
                */
            }
        }
    }
}
