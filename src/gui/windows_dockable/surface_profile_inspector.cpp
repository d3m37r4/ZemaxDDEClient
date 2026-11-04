#include <vector>
#include <string>
#include <algorithm>
#include <format>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/surface_profile_service.h"
#include "gui/imgui_utils.h"
#include "gui/notifications.h"
#include "gui/windows_dockable/calc_form_common.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void GuiManager::renderSurfaceProfileInspector() {
        auto& state = m_profileService->m_windowState;
        auto& nominal = m_profileService->m_nominalSurfaceData;
        auto& toleranced = m_profileService->m_tolerancedSurfaceData;
        const auto& sem = m_graphics.getThemeManager().semantic();
        const int hdrActiveIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

        ImGui::BeginChild("##SurfaceProfileContent",
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
        forms::RenderFrozenHint(m_profileService->getNominalCalcSlot(), hdrActiveIdx);
        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );
        {
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

                int nominalCalcSlotV = m_profileService->getNominalCalcSlot();
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

                if (ImGui::Button(ICON_FA_CHART_LINE " Show profile graphic")) {
                    m_profileService->m_showNominalProfileWindow = true;
                }

                ImGui::SameLine();

                ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_profileService->m_showNominalProfileWindow = false;
                    nominal.clear();
                }
                ImGuiUtils::PopFilledButton();
            } else {
                int nominalCalcSlot = m_profileService->getNominalCalcSlot();
                bool nominalFrozen = nominalCalcSlot >= 0 && nominalCalcSlot != activeIdx;
                bool nominalCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::NominalSurfaceProfile);
                bool nominalOnActiveSlot = nominalCalculating && nominalCalcSlot == activeIdx;

                ImGui::BeginDisabled(!isDDEInitialized() || nominalFrozen || nominalOnActiveSlot);

                forms::CalcFormContext form;
                {
                    const auto& optSys = nominalFrozen
                        ? m_profileService->m_frozenNominalOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_profileService->m_frozenNominalOpticalSystem);
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0;

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

                ImGui::BeginDisabled(!isDDEInitialized() || nominalCalculating);
                if (ImGui::Button(ICON_FA_COPY " Copy toleranced surface settings")) {
                    state.nominalSampling = state.tolerancedSampling;
                    state.nominalAngle = state.tolerancedAngle;
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                forms::RenderResultActions(
                    form, m_uiOpMonitor, sem, isDDEInitialized(), activeIdx,
                    [&]() {
                        if (isDDEInitialized()) {
                            m_profileService->m_frozenNominalOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();

                            m_profileService->onNominalCalculationComplete = [this, &nominal]() {
                                const auto& result = m_profileService->getResult(TaskSource::NominalSurfaceProfile);
                                nominal.id = result.id;
                                nominal.type = result.type;
                                nominal.units = result.units;
                                nominal.semiDiameter = result.semiDiameter;
                                nominal.sampling = result.sampling;
                                nominal.angle = result.angle;
                                nominal.sagDataPoints = result.sagDataPoints;
                                notify::success(std::format("Nominal profile ready: {} points",
                                                            nominal.sagDataPoints.size()));
                            };
                            m_profileService->startCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
                        }
                    },
                    [&]() {
                        m_profileService->cancelCalculation(TaskSource::NominalSurfaceProfile);
                        m_profileService->onNominalCalculationComplete = nullptr;
                        nominal.clear();
                    });
            }
        }
        ImGui::EndChild();

        ImGuiUtils::SectionHeader("Toleranced surface parameters");
        forms::RenderFrozenHint(m_profileService->getTolerancedCalcSlot(), hdrActiveIdx);
        ImGui::BeginChild(
            "TolerancedSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );
        {
            int activeIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

            if (toleranced.isValid() && toleranced.id == state.tolerancedSurfaceIndex) {
                ImGuiUtils::BeginPropertyGrid("##TolerancedData", maxLabelWidth);
                ImGuiUtils::PropertyGridRow("Optical system", toleranced.fileName.c_str());
                ImGuiUtils::PropertyGridRow("Surface", std::to_string(toleranced.id).c_str());
                ImGuiUtils::PropertyGridRow("Sampling", std::to_string(toleranced.sampling).c_str());
                ImGuiUtils::PropertyGridRow("Angle", std::format("{}°", toleranced.angle).c_str());
                ImGuiUtils::PropertyGridRow("Type", toleranced.type.c_str());
                ImGuiUtils::PropertyGridRow("Semi-diameter", std::format("{:.3f} {}", toleranced.semiDiameter, getUnitString(toleranced.units)).c_str());
                ImGuiUtils::PropertyGridRow("Diameter", std::format("{:.3f} {}", toleranced.diameter(), getUnitString(toleranced.units)).c_str());
                ImGuiUtils::EndPropertyGrid();
                ImGuiUtils::SpacingY(0.25f);

                int tolerancedCalcSlotV = m_profileService->getTolerancedCalcSlot();
                bool tolerancedFrozenV = tolerancedCalcSlotV >= 0 && tolerancedCalcSlotV != activeIdx;

                ImGui::BeginDisabled(tolerancedFrozenV);
                forms::RenderCalcBanner(TaskSource::TolerancedSurfaceProfile, "Toleranced Profile", m_uiOpMonitor, sem.warning);
                if (tolerancedFrozenV) {
                    ImGui::TextColored(sem.warning, "Frozen on slot %d: switch target (F6) or Cancel below.",
                        tolerancedCalcSlotV);
                }
                ImGui::EndDisabled();

                if (ImGui::Button(ICON_FA_FILE_EXPORT " Export TXT")) {
                    m_profileService->saveCrossSectionToFile(toleranced);
                }

                ImGui::SameLine();

                if (ImGui::Button(ICON_FA_CHART_LINE " Show profile graphic")) {
                    m_profileService->m_showTolerancedProfileWindow = true;
                }

                ImGui::SameLine();

                ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_profileService->m_showTolerancedProfileWindow = false;
                    toleranced.clear();
                }
                ImGuiUtils::PopFilledButton();
            } else {
                int tolerancedCalcSlot = m_profileService->getTolerancedCalcSlot();
                bool tolerancedFrozen = tolerancedCalcSlot >= 0 && tolerancedCalcSlot != activeIdx;
                bool tolerancedCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::TolerancedSurfaceProfile);
                bool tolerancedOnActiveSlot = tolerancedCalculating && tolerancedCalcSlot == activeIdx;

                ImGui::BeginDisabled(!isDDEInitialized() || tolerancedFrozen || tolerancedOnActiveSlot);

                forms::CalcFormContext form;
                {
                    const auto& optSys = tolerancedFrozen
                        ? m_profileService->m_frozenTolerancedOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_profileService->m_frozenTolerancedOpticalSystem);
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0;

                    form.source             = TaskSource::TolerancedSurfaceProfile;
                    form.bannerLabel        = "Toleranced Profile";
                    form.opticalSystem      = optSys.fileName;
                    form.copyId             = "##copy_optical_toleranced";
                    form.surfaceId          = "##toleranced_surf_num";
                    form.surfaceValue       = &state.tolerancedSurfaceIndex;
                    form.surfaceAdjustedAt  = &state.tolerancedSurfaceAdjustedAt;
                    form.samplingId         = "##toleranced_sampling";
                    form.samplingValue      = &state.tolerancedSampling;
                    form.samplingAdjustedAt = &state.tolerancedSamplingAdjustedAt;
                    form.angleId            = "##toleranced_angle";
                    form.angleValue         = &state.tolerancedAngle;
                    form.angleAdjustedAt    = &state.tolerancedAngleAdjustedAt;
                    form.maxSurfaces        = maxSurf;
                    form.frozen             = tolerancedFrozen;
                    form.onActiveSlot       = tolerancedOnActiveSlot;
                    form.actionLabel        = "Get toleranced surface data";
                    form.actionIcon         = ICON_FA_DOWNLOAD;

                    forms::RenderCalcForm(form, sem.danger, sem.warning);
                }

                ImGuiUtils::SpacingY(0.5f);

                if (tolerancedFrozen) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                    ImGui::TextColored(sem.warning, "Frozen on slot %d: switch target (F6) or Cancel below.",
                        tolerancedCalcSlot);
                }

                ImGui::EndDisabled();

                if (tolerancedOnActiveSlot) {
                    forms::RenderCalcBanner(form, m_uiOpMonitor, sem.warning);
                }

                ImGui::BeginDisabled(!isDDEInitialized() || tolerancedCalculating);
                if (ImGui::Button(ICON_FA_COPY " Copy nominal surface settings")) {
                    state.tolerancedSampling = state.nominalSampling;
                    state.tolerancedAngle = state.nominalAngle;
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                forms::RenderResultActions(
                    form, m_uiOpMonitor, sem, isDDEInitialized(), activeIdx,
                    [&]() {
                        if (isDDEInitialized()) {
                            m_profileService->m_frozenTolerancedOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();

                            m_profileService->onTolerancedCalculationComplete = [this, &toleranced]() {
                                const auto& result = m_profileService->getResult(TaskSource::TolerancedSurfaceProfile);
                                toleranced.id = result.id;
                                toleranced.type = result.type;
                                toleranced.units = result.units;
                                toleranced.semiDiameter = result.semiDiameter;
                                toleranced.sampling = result.sampling;
                                toleranced.angle = result.angle;
                                toleranced.sagDataPoints = result.sagDataPoints;
                                notify::success(std::format("Toleranced profile ready: {} points",
                                                            toleranced.sagDataPoints.size()));
                            };
                            m_profileService->startCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle, TaskSource::TolerancedSurfaceProfile);
                        }
                    },
                    [&]() {
                        m_profileService->cancelCalculation(TaskSource::TolerancedSurfaceProfile);
                        m_profileService->onTolerancedCalculationComplete = nullptr;
                        toleranced.clear();
                    });
            }
        }
        ImGui::EndChild();

        if (nominal.isValid() && toleranced.isValid()) {
            ImGuiUtils::SectionHeader("Surface profile analysis results");

            if (ImGui::Button(ICON_FA_CHART_LINE " Show comparison graphic")) {
                m_profileService->m_showComparisonProfileWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_FA_CHART_LINE " Show deviation graphic")) {
                m_profileService->m_showDeviationProfileWindow = true;
            }
        }

        ImGui::EndChild();
    }
}
