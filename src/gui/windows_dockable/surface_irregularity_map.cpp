#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

#include "gui/gui.h"
#include "gui/constants.h"
#include "dde/constants.h"
#include "gui/imgui_utils.h"
#include "logger/logger.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"

namespace {
    std::string getSamplingTooltip() {
        return std::format(
            "Total number of sample points across the full surface diameter (min={}, max={}).\n"
            "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
    }

    std::string getAngleStepTooltip() {
        return "Angular step between cross-section profiles in degrees.\n"
               "Profiles span 180° (each profile gives two opposite radial lines).\n"
               "Smaller step = more sections, smoother 3D surface, slower calculation.";
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderSurfaceIrregularityMap() {
        auto& state = m_irregularityMapService->m_windowState;

        ImGui::BeginChild("##SurfaceIrregularityMapContent",
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
            const auto& nominal = m_irregularityMapService->m_nominalSurfaceData;

            if (nominal.isValid() && nominal.id == state.nominalSurfaceIndex) {
                ImGuiUtils::BeginPropertyGrid("##NominalData", maxLabelWidth);
                ImGuiUtils::PropertyGridRow("Optical system", nominal.fileName.c_str());
                ImGuiUtils::PropertyGridRow("Surface", std::to_string(nominal.id).c_str());
                ImGuiUtils::PropertyGridRow("Sampling", std::to_string(nominal.sampling).c_str());
                ImGuiUtils::PropertyGridRow("Angle", std::format("{}°", nominal.angle).c_str());
                ImGuiUtils::PropertyGridRow("Type", nominal.type.c_str());
                ImGuiUtils::PropertyGridRow("Semi-diameter", std::format("{:.3f} {}", nominal.semiDiameter, getUnitString(nominal.units)).c_str());
                ImGuiUtils::PropertyGridRow("Diameter", std::format("{:.3f} {}", nominal.diameter(), getUnitString(nominal.units)).c_str());
                ImGuiUtils::EndPropertyGrid();
                ImGuiUtils::SpacingY(0.25f);

                if (ImGui::Button("Export txt")) {
                    m_profileService->saveCrossSectionToFile(nominal);
                }

                ImGui::SameLine();

                if (ImGui::Button("Clear data")) {
                    m_irregularityMapService->m_nominalSurfaceData.clear();
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
                        m_irregularityMapService->cancelCalculation();
                        m_irregularityMapService->onCalculationComplete = nullptr;
                        m_irregularityMapService->m_nominalSurfaceData.clear();
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
                            auto units = m_zemaxDDEClient->getOpticalSystemData().units;
                            auto fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                            m_irregularityMapService->onCalculationComplete = [this, units, fileName]() {
                                m_irregularityMapService->m_nominalSurfaceData.units = units;
                                m_irregularityMapService->m_nominalSurfaceData.fileName = fileName;
                                m_logger.addLog("[IrregularityMapService] Nominal reference set");
                            };
                            m_irregularityMapService->startCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
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
        state.tolerancedAngleStep = std::clamp(state.tolerancedAngleStep, 0.01, 180.0);
        ImGui::SameLine();
        ImGuiUtils::HelpMarker(getAngleStepTooltip().c_str());

        int numSections = state.tolerancedAngleStep > 0.0 ? static_cast<int>(180.0 / state.tolerancedAngleStep) : 0;
        ImGui::TextUnformatted("Number of sections:");
        ImGui::SameLine();
        ImGui::TextUnformatted(std::to_string(numSections).c_str());

        ImGui::EndDisabled();

        ImGui::EndChild();

        ImGui::SeparatorText("Analysis");

        if (m_uiOpMonitor.isActive(TaskSource::SurfaceIrregularityMap)) {
            ImGuiUtils::SpinnerButton("Processing...", true);
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                m_irregularityMapService->cancelMapCalculation();
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
                m_irregularityMapService->startMapCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngleStep);
            }
            ImGui::EndDisabled();
        }

        if (m_irregularityMapService->hasData()) {
            bool calculating = m_uiOpMonitor.isActive(TaskSource::SurfaceIrregularityMap);

            ImGui::SeparatorText("Results");

            ImGui::BeginDisabled(calculating);

            const auto& profiles = m_irregularityMapService->getProfiles();
            ImGui::Text(std::format("Sections calculated: {} ({}° step, {} pts each)",
                profiles.size(), state.tolerancedAngleStep, state.tolerancedSampling).c_str());

            auto& maxPV = m_irregularityMapService->getMaxPVResult();
            if (maxPV.has_value() && m_irregularityMapService->m_nominalSurfaceData.isValid()) {
                ImGui::Spacing();
                ImGui::TextUnformatted("Maximum P-V section:");
                ImGui::Text(std::format("  Angle: {:.2f}°", maxPV->angle).c_str());
                ImGui::Text(std::format("  Peak (P):  {:.6f} mm", maxPV->peak).c_str());
                ImGui::Text(std::format("  Valley (V): {:.6f} mm", maxPV->valley).c_str());
                ImGui::Text(std::format("  P-V:       {:.6f} mm", maxPV->pv).c_str());

                ImGui::Spacing();
                if (ImGui::Button("Show worst section profile")) {
                    for (const auto& p : profiles) {
                        if (std::abs(p.angle - maxPV->angle) < 0.01) {
                            m_irregularityMapService->m_worstProfileData = p;
                            m_irregularityMapService->m_showWorstSectionProfile = true;
                            break;
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Show deviation graphic")) {
                    if (!m_irregularityMapService->m_worstProfileData.isValid()) {
                        for (const auto& p : profiles) {
                            if (std::abs(p.angle - maxPV->angle) < 0.01) {
                                m_irregularityMapService->m_worstProfileData = p;
                                break;
                            }
                        }
                    }
                    m_irregularityMapService->m_showWorstSectionDeviation = true;
                }
            }

            ImGui::Spacing();

            if (ImGui::Button("Show 3D surface map")) {
                m_irregularityMapService->m_showTolerancedSurfaceMap = true;
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(!m_irregularityMapService->m_nominalSurfaceData.isValid());
            if (ImGui::Button("Show 3D deviation map")) {
                m_irregularityMapService->m_showDeviationSurfaceMap = true;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_irregularityMapService->clearData();
                m_irregularityMapService->m_showTolerancedSurfaceMap = false;
                m_irregularityMapService->m_showDeviationSurfaceMap = false;
                m_irregularityMapService->m_showWorstSectionProfile = false;
                m_irregularityMapService->m_showWorstSectionDeviation = false;
            }

            ImGui::EndDisabled();
        }

        ImGui::EndChild();
    }
}
