#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

#include "app/compute/surface_map.h"
#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/windows_dockable/calc_form_common.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "logger/logger.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"

namespace gui {
    void GuiManager::renderSurfaceIrregularityMap() {
        auto& state = m_irregularityMapService->m_windowState;
        const auto& sem = m_graphics.getThemeManager().semantic();
        const int hdrActiveIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

        ImGui::BeginChild("##SurfaceIrregularityMapContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle | ImGuiChildFlags_Borders,
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

        ImGuiUtils::SectionHeader("Nominal surface parameters");
        forms::RenderFrozenHint(m_irregularityMapService->getNominalCalcSlot(), hdrActiveIdx);

        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        {
            const auto& nominal = m_irregularityMapService->m_nominalSurfaceData;
            int activeIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

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

                int nominalCalcSlotV = m_irregularityMapService->getNominalCalcSlot();
                bool nominalFrozenV = nominalCalcSlotV >= 0 && nominalCalcSlotV != activeIdx;

                ImGui::BeginDisabled(nominalFrozenV);
                forms::RenderCalcBanner(TaskSource::NominalSurfaceProfile, "Nominal Profile", m_uiOpMonitor, sem.warning);
                if (nominalFrozenV) {
                    ImGui::TextColored(sem.warning, "Frozen on slot %d: switch target (F6) or Cancel below.",
                        nominalCalcSlotV);
                }
                ImGui::EndDisabled();

                if (ImGui::Button(ICON_FA_FILE_EXPORT " Export TXT")) {
                    m_profileService->saveCrossSectionToFile(nominal);
                }

                ImGui::SameLine();

                ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_irregularityMapService->m_nominalSurfaceData.clear();
                }
                ImGuiUtils::PopFilledButton();
            } else {
                int nominalCalcSlot = m_irregularityMapService->getNominalCalcSlot();
                bool nominalFrozen = nominalCalcSlot >= 0 && nominalCalcSlot != activeIdx;
                bool nominalCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::NominalSurfaceProfile);
                bool nominalOnActiveSlot = nominalCalculating && nominalCalcSlot == activeIdx;

                ImGui::BeginDisabled(!isDDEInitialized() || nominalFrozen || nominalOnActiveSlot);

                forms::CalcFormContext form;
                {
                    const auto& optSys = nominalFrozen
                        ? m_irregularityMapService->m_frozenNominalOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_irregularityMapService->m_frozenNominalOpticalSystem);
                    // Without a DDE client the surface count is unknown, so
                    // the clamp does not run at all (in the inspector it goes
                    // to 0).
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : -1;

                    form.source             = TaskSource::NominalSurfaceProfile;
                    form.bannerLabel        = "Nominal Profile";
                    form.opticalSystem      = optSys.fileName;
                    form.copyId             = "##copy_optical_nominal";
                    form.surfaceId          = "##nominal_surf_num";
                    form.surfaceValue       = &state.nominalSurfaceIndex;
                    form.surfaceAdjustedAt  = &state.nominalSurfaceAdjustedAt;
                    form.samplingId         = "##nominal_sampling";
                    form.samplingValue      = &state.nominalSampling;
                    form.samplingAdjustedAt = &state.nominalSamplingAdjustedAt;
                    form.angleId            = "##nominal_angle";
                    form.angleValue         = &state.nominalAngle;
                    form.angleAdjustedAt    = &state.nominalAngleAdjustedAt;
                    form.maxSurfaces        = maxSurf;
                    form.frozen             = nominalFrozen;
                    form.onActiveSlot       = nominalOnActiveSlot;
                    form.actionLabel        = "Get nominal surface data";
                    form.actionIcon         = ICON_FA_DOWNLOAD;

                    forms::RenderCalcForm(form, sem.danger, sem.warning);
                }

                ImGuiUtils::SpacingY(0.5f);

                if (nominalFrozen) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                    ImGui::TextColored(sem.warning, "Frozen on slot %d: switch target (F6) or Cancel below.",
                        nominalCalcSlot);
                }

                ImGui::EndDisabled();

                if (nominalOnActiveSlot) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                }

                forms::RenderResultActions(
                    form, m_uiOpMonitor, sem, isDDEInitialized(), activeIdx,
                    [&]() {
                        if (isDDEInitialized()) {
                            m_irregularityMapService->m_frozenNominalOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();
                            auto units = m_zemaxDDEClient->getOpticalSystemData().units;
                            auto fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                            m_irregularityMapService->onNominalCalculationComplete = [this, units, fileName]() {
                                m_irregularityMapService->m_nominalSurfaceData.units = units;
                                m_irregularityMapService->m_nominalSurfaceData.fileName = fileName;
                            };
                            m_irregularityMapService->startCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
                        }
                    },
                    [&]() {
                        m_irregularityMapService->cancelCalculation();
                        m_irregularityMapService->onNominalCalculationComplete = nullptr;
                        m_irregularityMapService->m_nominalSurfaceData.clear();
                    });
            }
        }

        ImGui::EndChild();

        ImGuiUtils::SectionHeader("Toleranced surface parameters");
        forms::RenderFrozenHint(m_irregularityMapService->getMapCalcSlot(), hdrActiveIdx);

        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        {
            int activeIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;
            int mapCalcSlot = m_irregularityMapService->getMapCalcSlot();
            bool mapFrozen = mapCalcSlot >= 0 && mapCalcSlot != activeIdx;
            bool mapCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::SurfaceIrregularityMap);
            bool mapOnActiveSlot = mapCalculating && mapCalcSlot == activeIdx;

            if (m_irregularityMapService->hasData()) {
                const auto& optSys = m_irregularityMapService->m_frozenMapOpticalSystem;
                const app::compute::MapPlan plan = app::compute::planSurfaceMap(
                    state.tolerancedAngleStep, state.tolerancedSampling);

                ImGuiUtils::BeginPropertyGrid("##TolerancedData", maxLabelWidth);
                ImGuiUtils::PropertyGridRow("Optical system", optSys.fileName.c_str());
                ImGuiUtils::PropertyGridRow("Surface", std::to_string(state.tolerancedSurfaceIndex).c_str());
                ImGuiUtils::PropertyGridRow("Sampling", std::to_string(state.tolerancedSampling).c_str());
                ImGuiUtils::PropertyGridRow("Angle step", std::format("{}°", state.tolerancedAngleStep).c_str());
                ImGuiUtils::PropertyGridRow("Sections", std::to_string(plan.sections).c_str());
                ImGuiUtils::EndPropertyGrid();
                ImGuiUtils::SpacingY(0.25f);

                if (ImGui::Button(ICON_FA_CHART_LINE " Show 3D surface map")) {
                    m_irregularityMapService->m_showTolerancedSurfaceMap = true;
                }

                ImGui::SameLine();

                ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_irregularityMapService->clearData();
                    m_irregularityMapService->m_showTolerancedSurfaceMap = false;
                    m_irregularityMapService->m_showDeviationSurfaceMap = false;
                    m_irregularityMapService->m_showWorstSectionProfile = false;
                    m_irregularityMapService->m_showWorstSectionDeviation = false;
                    m_irregularityMapService->m_worstProfileData.clear();
                }
                ImGuiUtils::PopFilledButton();
            } else {
                ImGui::BeginDisabled(!isDDEInitialized() || mapFrozen || mapOnActiveSlot);

                forms::CalcFormContext form;
                {
                    const auto& optSys = mapFrozen
                        ? m_irregularityMapService->m_frozenMapOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_irregularityMapService->m_frozenMapOpticalSystem);
                    // Without a DDE client the surface count is unknown, so
                    // the clamp does not run at all (in the inspector it goes
                    // to 0).
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : -1;

                    form.source             = TaskSource::SurfaceIrregularityMap;
                    form.bannerLabel        = "Surface Map";
                    form.opticalSystem      = optSys.fileName;
                    form.copyId             = "##copy_optical_map";
                    form.surfaceId          = "##toleranced_surf_num";
                    form.surfaceValue       = &state.tolerancedSurfaceIndex;
                    form.surfaceAdjustedAt  = &state.tolerancedSurfaceAdjustedAt;
                    form.samplingId         = "##toleranced_sampling";
                    form.samplingValue      = &state.tolerancedSampling;
                    form.samplingAdjustedAt = &state.tolerancedSamplingAdjustedAt;
                    form.angleId            = "##toleranced_angle_step";
                    form.angleValue         = &state.tolerancedAngleStep;
                    form.angleAdjustedAt    = &state.tolerancedAngleStepAdjustedAt;
                    form.angleIsStep        = true;
                    form.maxSurfaces        = maxSurf;
                    form.frozen             = mapFrozen;
                    form.onActiveSlot       = mapOnActiveSlot;
                    form.actionLabel        = "Calculate Surface Map";
                    form.actionIcon         = ICON_FA_CALCULATOR;

                    forms::RenderCalcForm(form, sem.danger, sem.warning);
                }

                const app::compute::MapPlan plan = app::compute::planSurfaceMap(
                    state.tolerancedAngleStep, state.tolerancedSampling);
                ImGui::TextUnformatted("Number of sections:");
                ImGui::SameLine();
                ImGui::TextUnformatted(std::to_string(plan.sections).c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("·");
                ImGui::SameLine();
                ImGui::TextUnformatted("Estimated DDE requests:");
                ImGui::SameLine();
                ImGui::TextUnformatted(gui::formatCount(plan.estimatedRequests).c_str());
                if (plan.adjusted) {
                    ImGui::TextColored(sem.warning, "%s",
                        std::format("Angle step raised to {:.3f}° to stay within the request budget",
                            plan.stepDeg).c_str());
                }

                ImGuiUtils::SpacingY(0.5f);

                if (mapFrozen) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                    ImGui::TextColored(sem.warning, "Frozen on slot %d: switch target (F6) or Cancel below.",
                        mapCalcSlot);
                }

                ImGui::EndDisabled();

                if (mapOnActiveSlot) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                }

                forms::RenderResultActions(
                    form, m_uiOpMonitor, sem, isDDEInitialized(), activeIdx,
                    [&]() {
                        if (m_zemaxDDEClient) {
                            m_irregularityMapService->m_frozenMapOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();
                        }
                        m_irregularityMapService->startMapCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngleStep);
                    },
                    [&]() {
                        m_irregularityMapService->cancelMapCalculation();
                    });
            }
        }

        ImGui::EndChild();

        if (m_irregularityMapService->hasData()) {
            bool calculating = m_uiOpMonitor.hasActiveTasksOnSlot(
                m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1,
                TaskSource::SurfaceIrregularityMap);

            ImGuiUtils::SectionHeader("Analysis");

            ImGui::BeginDisabled(calculating);

            const auto& profiles = m_irregularityMapService->getProfiles();
            ImGui::Text(std::format("Sections available for analysis: {} ({:.1f}° step, {} pts each)",
                profiles.size(), state.tolerancedAngleStep, state.tolerancedSampling).c_str());

            auto& maxPV = m_irregularityMapService->getMaxPVResult();
            bool canCalculate = m_irregularityMapService->m_nominalSurfaceData.isValid() && !maxPV.has_value();

            ImGui::BeginDisabled(!canCalculate);
            if (ImGui::Button(ICON_FA_MAGNIFYING_GLASS " Find MaxPV (Profile deviation)")) {
                m_irregularityMapService->calculateDeviations();
            }
            ImGui::EndDisabled();

            if (maxPV.has_value() && m_irregularityMapService->m_nominalSurfaceData.isValid()) {
                ImGui::Spacing();
                ImGui::TextUnformatted("Maximum P-V section:");
                ImGui::Text(std::format("  Angle: {:.2f}°", maxPV->angle).c_str());
                ImGui::Text(std::format("  Peak (P):  {:.6f} mm", maxPV->peak).c_str());
                ImGui::Text(std::format("  Valley (V): {:.6f} mm", maxPV->valley).c_str());
                ImGui::Text(std::format("  P-V:       {:.6f} mm", maxPV->pv).c_str());

                ImGui::Spacing();
                if (ImGui::Button(ICON_FA_CHART_LINE " Show worst profile")) {
                    for (const auto& p : profiles) {
                        if (std::abs(p.angle - maxPV->angle) < 0.01) {
                            m_irregularityMapService->m_worstProfileData = p;
                            m_irregularityMapService->m_showWorstSectionProfile = true;
                            break;
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_CHART_LINE " Show deviation graphic")) {
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

            ImGui::BeginDisabled(!m_irregularityMapService->m_nominalSurfaceData.isValid() || !maxPV.has_value());
            if (ImGui::Button(ICON_FA_CHART_LINE " Show 3D deviation map")) {
                m_irregularityMapService->m_showDeviationSurfaceMap = true;
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
            if (ImGui::Button(ICON_FA_TRASH " Clear all")) {
                m_irregularityMapService->m_nominalSurfaceData.clear();
                m_irregularityMapService->clearData();
                m_irregularityMapService->m_showTolerancedSurfaceMap = false;
                m_irregularityMapService->m_showDeviationSurfaceMap = false;
                m_irregularityMapService->m_showWorstSectionProfile = false;
                m_irregularityMapService->m_showWorstSectionDeviation = false;
                m_irregularityMapService->m_worstProfileData.clear();
            }
            ImGuiUtils::PopFilledButton();

            ImGui::EndDisabled();
        }

        ImGui::EndChild();
    }
}
