#include <algorithm>
#include <cmath>
#include <format>
#include <vector>
#include <numbers>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "logger/logger.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;

    std::string getSamplingTooltip() {
        return std::format(
            "Total number of sample points across the full surface diameter (min={}, max={}).\n"
            "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
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
        auto& state = m_sagMapService->m_state;

        ImGui::BeginChild("##SurfaceMapContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        float cp = ImGui::GetStyle().CellPadding.x;
        const char* dataLabels[] = {
            "Optical system", "Surface", "Sampling", "Angle",
            "Type", "Semi-diameter", "Diameter"
        };
        float maxLabelWidth = 0.0f;
        for (auto* l : dataLabels)
            maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(l).x);
        maxLabelWidth += cp * 2.0f;

        ImGui::SeparatorText("Nominal surface parameters");

        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        {
            auto* nominal = m_zemaxDDEClient ? m_zemaxDDEClient->getNominalSurface() : nullptr;

            if (nominal && nominal->isValid() && nominal->id == state.nominalSurfaceIndex) {
                ImGuiUtils::BeginPropertyGrid("##NominalData", maxLabelWidth);
                ImGuiUtils::PropertyGridRow("Optical system", nominal->fileName.c_str());
                ImGuiUtils::PropertyGridRow("Surface", std::to_string(nominal->id).c_str());
                ImGuiUtils::PropertyGridRow("Sampling", std::to_string(nominal->sampling).c_str());
                ImGuiUtils::PropertyGridRow("Angle", std::format("{}°", nominal->angle).c_str());
                ImGuiUtils::PropertyGridRow("Type", nominal->type.c_str());
                ImGuiUtils::PropertyGridRow("Semi-diameter", std::format("{:.3f} {}", nominal->semiDiameter, getUnitString(nominal->units)).c_str());
                ImGuiUtils::PropertyGridRow("Diameter", std::format("{:.3f} {}", nominal->diameter(), getUnitString(nominal->units)).c_str());
                ImGuiUtils::EndPropertyGrid();
                ImGuiUtils::SpacingY(0.25f);

                if (ImGui::Button("Export txt")) {
                    m_profileService->saveCrossSectionToFile(*nominal);
                }

                ImGui::SameLine();

                if (ImGui::Button("Clear data")) {
                    nominal->clear();
                }
            } else {
                if (!isDDEInitialized()) {
                    ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
                    ImGui::Spacing();
                }

                ImGui::BeginDisabled(!isDDEInitialized());

                {
                    auto fileName = m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().fileName : std::string{};
                    ImGui::TextUnformatted("Optical system:");
                    ImGui::SameLine();
                    ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
                }

                ImGui::TextUnformatted("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
                if (m_zemaxDDEClient)
                    state.nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.nominalSurfaceIndex));

                ImGui::TextUnformatted("Sampling:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                ImGui::InputInt("##nominal_sampling", &state.nominalSampling, 10, 50);
                state.nominalSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.nominalSampling)) | 1;
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());

                ImGui::TextUnformatted("Angle:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                ImGui::InputDouble("##nominal_angle", &state.nominalAngle, 1.0, 10.0, "%.2f");
                state.nominalAngle = std::clamp(state.nominalAngle, -360.0, 360.0);
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getAngleTooltip().c_str());

                ImGuiUtils::SpacingY(0.5f);

                if (m_uiOpMonitor.isActive(TaskSource::NominalSurfaceProfile)) {
                    ImGuiUtils::SpinnerButton("Processing...", true);
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        m_profileService->cancelCalculation();
                        m_profileService->onCalculationComplete = nullptr;
                        m_zemaxDDEClient->getNominalSurface()->clear();
                    }
                } else if (m_uiOpMonitor.hasActiveTasks()) {
                    ImGui::BeginDisabled(true);
                    ImGui::Button("Get nominal surface data");
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::TextUnformatted("Another calculation is in progress");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndDisabled();
                } else {
                    if (ImGui::Button("Get nominal surface data")) {
                        if (isDDEInitialized()) {
                            auto* nominal = m_zemaxDDEClient->getNominalSurface();
                            nominal->units = m_zemaxDDEClient->getOpticalSystemData().units;
                            nominal->fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                            m_profileService->onCalculationComplete = [this]() {
                                if (!m_zemaxDDEClient) return;
                                const auto& result = m_profileService->getResult();
                                auto* nominal = m_zemaxDDEClient->getNominalSurface();
                                nominal->semiDiameter = result.semiDiameter;
                                nominal->sampling = result.sampling;
                                nominal->angle = result.angle;
                                nominal->sagDataPoints = result.sagDataPoints;
                                m_logger.addLog("[SagMap] Nominal reference set for surface map analysis");
                            };
                            m_profileService->startAsyncSagCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
                        }
                    }
                }

                ImGui::EndDisabled();
            }
        }

        ImGui::EndChild();

        ImGui::SeparatorText("Toleranced surface parameters");

        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        if (!isDDEInitialized()) {
            ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
            ImGui::Spacing();
        }

        ImGui::BeginDisabled(!isDDEInitialized());

        {
            auto fileName = m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().fileName : std::string{};
            ImGui::TextUnformatted("Optical system:");
            ImGui::SameLine();
            ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
        }

        ImGui::TextUnformatted("Surface number:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
        if (m_zemaxDDEClient)
            state.tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, state.tolerancedSurfaceIndex));

        ImGui::TextUnformatted("Sampling:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
        state.tolerancedSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.tolerancedSampling)) | 1;
        ImGui::SameLine();
        ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());

        ImGui::TextUnformatted("Angle step:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputDouble("##toleranced_angle_step", &state.tolerancedAngleStep, 0.1, 1.0, "%.2f");
        state.tolerancedAngleStep = std::clamp(state.tolerancedAngleStep, 0.01, 360.0);
        ImGui::SameLine();
        ImGuiUtils::HelpMarker(getAngleStepTooltip().c_str());

        ImGui::EndDisabled();

        ImGui::EndChild();

        ImGui::SeparatorText("Analysis");

        if (m_uiOpMonitor.isActive(TaskSource::SurfaceMapAnalysis)) {
            ImGuiUtils::SpinnerButton("Processing...", true);
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                m_sagMapService->cancelCalculation();
                m_sagMapService->onCalculationComplete = nullptr;
                m_sagMapService->clearData();
            }
        } else if (m_uiOpMonitor.hasActiveTasks()) {
            ImGui::BeginDisabled(true);
            ImGui::Button("Calculate Surface Map");
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextUnformatted("Another calculation is in progress");
                ImGui::EndTooltip();
            }
            ImGui::EndDisabled();
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

        ImGui::EndChild();
    }
}
