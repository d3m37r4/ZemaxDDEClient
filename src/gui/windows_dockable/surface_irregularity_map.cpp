#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "app/utils.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
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

    // Right-aligned (?) after a section header, visible only while this
    // section's calculation source is frozen on another slot.
    void frozenSectionHelp(int calcSlot, int activeIdx) {
        if (calcSlot < 0 || calcSlot == activeIdx) return;
        ImGui::SameLine();
        ImGuiUtils::HelpMarker(
            "Frozen form: values were captured from the slot where the calculation started. "
            "Switch target (F6) to edit, or Cancel the running calculation.");
    }

    void renderCalcBanner(app::models::TaskSource source, const char* label, gui::UiOperationMonitor& monitor) {
        auto progress = monitor.getTaskProgress(source);
        if (!progress) return;
        int pct = progress->totalSteps > 0 ? (progress->currentStep * 100 / progress->totalSteps) : 0;
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s — calculation in progress on slot %d: %d%%", label, progress->clientSlot, pct);
    }
}

namespace gui {
    void GuiManager::renderSurfaceIrregularityMap() {
        auto& state = m_irregularityMapService->m_windowState;
        const auto& sem = m_graphics.getThemeManager().semantic();
        const int hdrActiveIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

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

        ImGuiUtils::SectionHeader("Nominal surface parameters");
        frozenSectionHelp(m_irregularityMapService->getNominalCalcSlot(), hdrActiveIdx);

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
                renderCalcBanner(TaskSource::NominalSurfaceProfile, "Nominal Profile", m_uiOpMonitor);
                if (nominalFrozenV) {
                    ImGui::TextDisabled("Frozen on slot %d: switch target (F6) or Cancel below.",
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

                {
                    const auto& optSys = nominalFrozen
                        ? m_irregularityMapService->m_frozenNominalOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_irregularityMapService->m_frozenNominalOpticalSystem);
                    auto fileName = optSys.fileName;
                    ImGui::TextUnformatted("Optical system:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-(ImGuiUtils::CopyButtonWidth()
                        + ImGui::GetStyle().ItemSpacing.x));
                    ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    ImGuiUtils::CopyButton(ICON_FA_COPY "##copy_optical_nominal", fileName.c_str());
                }

                ImGui::TextUnformatted("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool surfFlash = ImGuiUtils::AdjustedFlashActive(
                    state.nominalSurfaceAdjustedAt);
                const bool surfTint = ImGuiUtils::BeginAdjustedHighlight(surfFlash, sem.danger);
                ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
                if (surfTint) ImGui::PopStyleColor();
                if (!nominalFrozen && !nominalOnActiveSlot && m_zemaxDDEClient) {
                    const int maxSurf = m_zemaxDDEClient->getOpticalSystemData().numSurfs;
                    if (app::ClampWithFeedback(state.nominalSurfaceIndex, 0, maxSurf))
                        state.nominalSurfaceAdjustedAt = ImGui::GetTime();
                    ImGuiUtils::AdjustedHint(surfFlash,
                        std::format("Adjusted to 0-{}", maxSurf).c_str(), sem.warning);
                }

                ImGui::TextUnformatted("Sampling:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool sampFlash = ImGuiUtils::AdjustedFlashActive(
                    state.nominalSamplingAdjustedAt);
                const bool sampTint = ImGuiUtils::BeginAdjustedHighlight(sampFlash, sem.danger);
                ImGui::InputInt("##nominal_sampling", &state.nominalSampling, 10, 50);
                if (sampTint) ImGui::PopStyleColor();
                if (app::ClampWithFeedback(
                        state.nominalSampling, gui::MIN_SAMPLING, gui::MAX_SAMPLING))
                    state.nominalSamplingAdjustedAt = ImGui::GetTime();
                if (app::SnapOddWithFeedback(state.nominalSampling))
                    state.nominalSamplingAdjustedAt = ImGui::GetTime();
                app::ClampWithFeedback(state.nominalSampling, gui::MIN_SAMPLING, gui::MAX_SAMPLING);
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());
                ImGuiUtils::AdjustedHint(sampFlash,
                    std::format("Adjusted to {}-{} (odd)",
                        gui::MIN_SAMPLING, gui::MAX_SAMPLING).c_str(), sem.warning);

                ImGui::TextUnformatted("Angle:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool angFlash = ImGuiUtils::AdjustedFlashActive(
                    state.nominalAngleAdjustedAt);
                const bool angTint = ImGuiUtils::BeginAdjustedHighlight(angFlash, sem.danger);
                ImGui::InputDouble("##nominal_angle", &state.nominalAngle, 1.0, 10.0, "%.2f");
                if (angTint) ImGui::PopStyleColor();
                if (app::ClampWithFeedback(state.nominalAngle, -360.0, 360.0))
                    state.nominalAngleAdjustedAt = ImGui::GetTime();
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getAngleTooltip().c_str());
                ImGuiUtils::AdjustedHint(angFlash,
                    "Adjusted to -360 - 360", sem.warning);

                ImGuiUtils::SpacingY(0.5f);

                if (nominalFrozen) {
                    renderCalcBanner(TaskSource::NominalSurfaceProfile, "Nominal Profile", m_uiOpMonitor);
                    ImGui::TextDisabled("Frozen on slot %d: switch target (F6) or Cancel below.",
                        nominalCalcSlot);
                }

                ImGui::EndDisabled();

                if (nominalOnActiveSlot) {
                    renderCalcBanner(TaskSource::NominalSurfaceProfile, "Nominal Profile", m_uiOpMonitor);
                }

                if (nominalFrozen || nominalOnActiveSlot) {
                    ImGuiUtils::SpinnerButton("Processing...", true);
                    ImGui::SameLine();
                    ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
                        m_irregularityMapService->cancelCalculation();
                        m_irregularityMapService->onNominalCalculationComplete = nullptr;
                        m_irregularityMapService->m_nominalSurfaceData.clear();
                    }
                    ImGuiUtils::PopFilledButton();
                } else if (m_uiOpMonitor.hasActiveTasksOnSlot(activeIdx)) {
                    ImGui::BeginDisabled(true);
                    ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
                    ImGui::Button(ICON_FA_DOWNLOAD " Get nominal surface data");
                    ImGuiUtils::PopFilledButton();
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::TextUnformatted("Another calculation is in progress on this slot");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndDisabled();
                } else {
                    ImGui::BeginDisabled(!isDDEInitialized());
                    ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
                    if (ImGui::Button(ICON_FA_DOWNLOAD " Get nominal surface data")) {
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
                    }
                    ImGuiUtils::PopFilledButton();
                    ImGui::EndDisabled();
                }
            }
        }

        ImGui::EndChild();

        ImGuiUtils::SectionHeader("Toleranced surface parameters");
        frozenSectionHelp(m_irregularityMapService->getMapCalcSlot(), hdrActiveIdx);

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
                int numSections = state.tolerancedAngleStep > 0.0 ? static_cast<int>(180.0 / state.tolerancedAngleStep) : 0;

                ImGuiUtils::BeginPropertyGrid("##TolerancedData", maxLabelWidth);
                ImGuiUtils::PropertyGridRow("Optical system", optSys.fileName.c_str());
                ImGuiUtils::PropertyGridRow("Surface", std::to_string(state.tolerancedSurfaceIndex).c_str());
                ImGuiUtils::PropertyGridRow("Sampling", std::to_string(state.tolerancedSampling).c_str());
                ImGuiUtils::PropertyGridRow("Angle step", std::format("{}°", state.tolerancedAngleStep).c_str());
                ImGuiUtils::PropertyGridRow("Sections", std::to_string(numSections).c_str());
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

                {
                    const auto& optSys = mapFrozen
                        ? m_irregularityMapService->m_frozenMapOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_irregularityMapService->m_frozenMapOpticalSystem);
                    auto fileName = optSys.fileName;
                    ImGui::TextUnformatted("Optical system:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-(ImGuiUtils::CopyButtonWidth()
                        + ImGui::GetStyle().ItemSpacing.x));
                    ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    ImGuiUtils::CopyButton(ICON_FA_COPY "##copy_optical_map", fileName.c_str());
                }

                ImGui::TextUnformatted("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool surfFlash = ImGuiUtils::AdjustedFlashActive(
                    state.tolerancedSurfaceAdjustedAt);
                const bool surfTint = ImGuiUtils::BeginAdjustedHighlight(surfFlash, sem.danger);
                ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
                if (surfTint) ImGui::PopStyleColor();
                if (!mapFrozen && !mapOnActiveSlot && m_zemaxDDEClient) {
                    const int maxSurf = m_zemaxDDEClient->getOpticalSystemData().numSurfs;
                    if (app::ClampWithFeedback(state.tolerancedSurfaceIndex, 0, maxSurf))
                        state.tolerancedSurfaceAdjustedAt = ImGui::GetTime();
                    ImGuiUtils::AdjustedHint(surfFlash,
                        std::format("Adjusted to 0-{}", maxSurf).c_str(), sem.warning);
                }

                ImGui::TextUnformatted("Sampling:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool sampFlash = ImGuiUtils::AdjustedFlashActive(
                    state.tolerancedSamplingAdjustedAt);
                const bool sampTint = ImGuiUtils::BeginAdjustedHighlight(sampFlash, sem.danger);
                ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
                if (sampTint) ImGui::PopStyleColor();
                if (app::ClampWithFeedback(
                        state.tolerancedSampling, gui::MIN_SAMPLING, gui::MAX_SAMPLING))
                    state.tolerancedSamplingAdjustedAt = ImGui::GetTime();
                if (app::SnapOddWithFeedback(state.tolerancedSampling))
                    state.tolerancedSamplingAdjustedAt = ImGui::GetTime();
                app::ClampWithFeedback(state.tolerancedSampling, gui::MIN_SAMPLING, gui::MAX_SAMPLING);
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());
                ImGuiUtils::AdjustedHint(sampFlash,
                    std::format("Adjusted to {}-{} (odd)",
                        gui::MIN_SAMPLING, gui::MAX_SAMPLING).c_str(), sem.warning);

                ImGui::TextUnformatted("Angle step:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool stepFlash = ImGuiUtils::AdjustedFlashActive(
                    state.tolerancedAngleStepAdjustedAt);
                const bool stepTint = ImGuiUtils::BeginAdjustedHighlight(stepFlash, sem.danger);
                ImGui::InputDouble("##toleranced_angle_step", &state.tolerancedAngleStep, 0.1, 1.0, "%.2f");
                if (stepTint) ImGui::PopStyleColor();
                if (app::ClampWithFeedback(state.tolerancedAngleStep, 0.01, 180.0))
                    state.tolerancedAngleStepAdjustedAt = ImGui::GetTime();
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getAngleStepTooltip().c_str());
                ImGuiUtils::AdjustedHint(stepFlash,
                    "Adjusted to 0.01-180", sem.warning);

                int numSections = state.tolerancedAngleStep > 0.0 ? static_cast<int>(180.0 / state.tolerancedAngleStep) : 0;
                ImGui::TextUnformatted("Number of sections:");
                ImGui::SameLine();
                ImGui::TextUnformatted(std::to_string(numSections).c_str());

                ImGuiUtils::SpacingY(0.5f);

                if (mapFrozen) {
                    renderCalcBanner(TaskSource::SurfaceIrregularityMap, "Surface Map", m_uiOpMonitor);
                    ImGui::TextDisabled("Frozen on slot %d: switch target (F6) or Cancel below.",
                        mapCalcSlot);
                }

                ImGui::EndDisabled();

                if (mapOnActiveSlot) {
                    renderCalcBanner(TaskSource::SurfaceIrregularityMap, "Surface Map", m_uiOpMonitor);
                }

                if (mapFrozen || mapOnActiveSlot) {
                    ImGuiUtils::SpinnerButton("Processing...", true);
                    ImGui::SameLine();
                    ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
                        m_irregularityMapService->cancelMapCalculation();
                    }
                    ImGuiUtils::PopFilledButton();
                } else if (m_uiOpMonitor.hasActiveTasksOnSlot(activeIdx)) {
                    ImGui::BeginDisabled(true);
                    ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
                    ImGui::Button(ICON_FA_CALCULATOR " Calculate Surface Map");
                    ImGuiUtils::PopFilledButton();
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::TextUnformatted("Another calculation is in progress on this slot");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndDisabled();
                } else {
                    ImGui::BeginDisabled(!isDDEInitialized());
                    ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
                    if (ImGui::Button(ICON_FA_CALCULATOR " Calculate Surface Map")) {
                        if (m_zemaxDDEClient) {
                            m_irregularityMapService->m_frozenMapOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();
                        }
                        m_irregularityMapService->startMapCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngleStep);
                    }
                    ImGuiUtils::PopFilledButton();
                    ImGui::EndDisabled();
                }
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
