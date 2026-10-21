#include <vector>
#include <string>
#include <algorithm>
#include <format>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/surface_profile_service.h"
#include "gui/imgui_utils.h"
#include "app/utils.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"

namespace {
    std::string getSamplingTooltip() {
        return std::format(
            "Total number of sample points across the full surface diameter (min={}, max={}).\n"
            "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
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
    void GuiManager::renderSurfaceProfileInspector() {
        auto& state = m_profileService->m_windowState;
        auto& nominal = m_profileService->m_nominalSurfaceData;
        auto& toleranced = m_profileService->m_tolerancedSurfaceData;
        const auto& sem = m_graphics.getThemeManager().semantic();
        const int hdrActiveIdx = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveIndex() : -1;

        ImGui::BeginChild("##SurfaceProfileContent",
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
        frozenSectionHelp(m_profileService->getNominalCalcSlot(), hdrActiveIdx);
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

                if (ImGui::Button(ICON_FA_CHART_LINE " Show profile graphic")) {
                    m_profileService->m_showNominalProfileWindow = true;
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button, sem.dangerButton);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.dangerButtonActive);
                ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_profileService->m_showNominalProfileWindow = false;
                    nominal.clear();
                }
                ImGui::PopStyleColor(4);
            } else {
                int nominalCalcSlot = m_profileService->getNominalCalcSlot();
                bool nominalFrozen = nominalCalcSlot >= 0 && nominalCalcSlot != activeIdx;
                bool nominalCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::NominalSurfaceProfile);
                bool nominalOnActiveSlot = nominalCalculating && nominalCalcSlot == activeIdx;

                ImGui::BeginDisabled(!isDDEInitialized() || nominalFrozen || nominalOnActiveSlot);

                {
                    const auto& optSys = nominalFrozen
                        ? m_profileService->m_frozenNominalOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_profileService->m_frozenNominalOpticalSystem);
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
                if (!nominalFrozen && !nominalOnActiveSlot) {
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0;
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

                ImGui::BeginDisabled(!isDDEInitialized() || nominalCalculating);
                if (ImGui::Button(ICON_FA_COPY " Copy toleranced surface settings")) {
                    state.nominalSampling = state.tolerancedSampling;
                    state.nominalAngle = state.tolerancedAngle;
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                if (nominalFrozen || nominalOnActiveSlot) {
                    ImGuiUtils::SpinnerButton("Processing...", true);
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.dangerButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.dangerButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
                        m_profileService->cancelCalculation(TaskSource::NominalSurfaceProfile);
                        m_profileService->onNominalCalculationComplete = nullptr;
                        nominal.clear();
                    }
                    ImGui::PopStyleColor(4);
                } else if (m_uiOpMonitor.hasActiveTasksOnSlot(activeIdx)) {
                    ImGui::BeginDisabled(true);
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.successButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.successButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.successButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    ImGui::Button(ICON_FA_DOWNLOAD " Get nominal surface data");
                    ImGui::PopStyleColor(4);
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::TextUnformatted("Another calculation is in progress on this slot");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndDisabled();
                } else {
                    ImGui::BeginDisabled(!isDDEInitialized());
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.successButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.successButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.successButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    if (ImGui::Button(ICON_FA_DOWNLOAD " Get nominal surface data")) {
                        if (isDDEInitialized()) {
                            m_profileService->m_frozenNominalOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();
                            nominal.units = m_zemaxDDEClient->getOpticalSystemData().units;
                            nominal.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                            m_profileService->onNominalCalculationComplete = [this, &nominal]() {
                                const auto& result = m_profileService->getResult(TaskSource::NominalSurfaceProfile);
                                nominal.id = result.id;
                                nominal.type = result.type;
                                nominal.units = result.units;
                                nominal.semiDiameter = result.semiDiameter;
                                nominal.sampling = result.sampling;
                                nominal.angle = result.angle;
                                nominal.sagDataPoints = result.sagDataPoints;
                            };
                            m_profileService->startCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
                        }
                    }
                    ImGui::PopStyleColor(4);
                    ImGui::EndDisabled();
                }
            }
        }
        ImGui::EndChild();

        ImGuiUtils::SectionHeader("Toleranced surface parameters");
        frozenSectionHelp(m_profileService->getTolerancedCalcSlot(), hdrActiveIdx);
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
                renderCalcBanner(TaskSource::TolerancedSurfaceProfile, "Toleranced Profile", m_uiOpMonitor);
                if (tolerancedFrozenV) {
                    ImGui::TextDisabled("Frozen on slot %d: switch target (F6) or Cancel below.",
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

                ImGui::PushStyleColor(ImGuiCol_Button, sem.dangerButton);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.dangerButtonActive);
                ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                if (ImGui::Button(ICON_FA_TRASH " Clear data")) {
                    m_profileService->m_showTolerancedProfileWindow = false;
                    toleranced.clear();
                }
                ImGui::PopStyleColor(4);
            } else {
                int tolerancedCalcSlot = m_profileService->getTolerancedCalcSlot();
                bool tolerancedFrozen = tolerancedCalcSlot >= 0 && tolerancedCalcSlot != activeIdx;
                bool tolerancedCalculating = m_uiOpMonitor.hasActiveTasks(TaskSource::TolerancedSurfaceProfile);
                bool tolerancedOnActiveSlot = tolerancedCalculating && tolerancedCalcSlot == activeIdx;

                ImGui::BeginDisabled(!isDDEInitialized() || tolerancedFrozen || tolerancedOnActiveSlot);

                {
                    const auto& optSys = tolerancedFrozen
                        ? m_profileService->m_frozenTolerancedOpticalSystem
                        : (m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData() : m_profileService->m_frozenTolerancedOpticalSystem);
                    auto fileName = optSys.fileName;
                    ImGui::TextUnformatted("Optical system:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-(ImGuiUtils::CopyButtonWidth()
                        + ImGui::GetStyle().ItemSpacing.x));
                    ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
                    ImGui::SameLine();
                    ImGuiUtils::CopyButton(ICON_FA_COPY "##copy_optical_toleranced", fileName.c_str());
                }

                ImGui::TextUnformatted("Surface number:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool surfFlash = ImGuiUtils::AdjustedFlashActive(
                    state.tolerancedSurfaceAdjustedAt);
                const bool surfTint = ImGuiUtils::BeginAdjustedHighlight(surfFlash, sem.danger);
                ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
                if (surfTint) ImGui::PopStyleColor();
                if (!tolerancedFrozen && !tolerancedOnActiveSlot) {
                    const int maxSurf = m_zemaxDDEClient
                        ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0;
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

                ImGui::TextUnformatted("Angle:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
                const bool angFlash = ImGuiUtils::AdjustedFlashActive(
                    state.tolerancedAngleAdjustedAt);
                const bool angTint = ImGuiUtils::BeginAdjustedHighlight(angFlash, sem.danger);
                ImGui::InputDouble("##toleranced_angle", &state.tolerancedAngle, 1.0, 10.0, "%.2f");
                if (angTint) ImGui::PopStyleColor();
                if (app::ClampWithFeedback(state.tolerancedAngle, -360.0, 360.0))
                    state.tolerancedAngleAdjustedAt = ImGui::GetTime();
                ImGui::SameLine();
                ImGuiUtils::HelpMarker(getAngleTooltip().c_str());
                ImGuiUtils::AdjustedHint(angFlash,
                    "Adjusted to -360 - 360", sem.warning);

                ImGuiUtils::SpacingY(0.5f);

                if (tolerancedFrozen) {
                    renderCalcBanner(TaskSource::TolerancedSurfaceProfile, "Toleranced Profile", m_uiOpMonitor);
                    ImGui::TextDisabled("Frozen on slot %d: switch target (F6) or Cancel below.",
                        tolerancedCalcSlot);
                }

                ImGui::EndDisabled();

                if (tolerancedOnActiveSlot) {
                    renderCalcBanner(TaskSource::TolerancedSurfaceProfile, "Toleranced Profile", m_uiOpMonitor);
                }

                ImGui::BeginDisabled(!isDDEInitialized() || tolerancedCalculating);
                if (ImGui::Button(ICON_FA_COPY " Copy nominal surface settings")) {
                    state.tolerancedSampling = state.nominalSampling;
                    state.tolerancedAngle = state.nominalAngle;
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                if (tolerancedFrozen || tolerancedOnActiveSlot) {
                    ImGuiUtils::SpinnerButton("Processing...", true);
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.dangerButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.dangerButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.dangerButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
                        m_profileService->cancelCalculation(TaskSource::TolerancedSurfaceProfile);
                        m_profileService->onTolerancedCalculationComplete = nullptr;
                        toleranced.clear();
                    }
                    ImGui::PopStyleColor(4);
                } else if (m_uiOpMonitor.hasActiveTasksOnSlot(activeIdx)) {
                    ImGui::BeginDisabled(true);
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.successButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.successButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.successButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    ImGui::Button(ICON_FA_DOWNLOAD " Get toleranced surface data");
                    ImGui::PopStyleColor(4);
                    if (ImGui::BeginItemTooltip()) {
                        ImGui::TextUnformatted("Another calculation is in progress on this slot");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndDisabled();
                } else {
                    ImGui::BeginDisabled(!isDDEInitialized());
                    ImGui::PushStyleColor(ImGuiCol_Button, sem.successButton);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sem.successButtonHover);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, sem.successButtonActive);
                    ImGui::PushStyleColor(ImGuiCol_Text, sem.onAccent);
                    if (ImGui::Button(ICON_FA_DOWNLOAD " Get toleranced surface data")) {
                        if (isDDEInitialized()) {
                            m_profileService->m_frozenTolerancedOpticalSystem = m_zemaxDDEClient->getOpticalSystemData();
                            toleranced.units = m_zemaxDDEClient->getOpticalSystemData().units;
                            toleranced.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                            m_profileService->onTolerancedCalculationComplete = [this, &toleranced]() {
                                const auto& result = m_profileService->getResult(TaskSource::TolerancedSurfaceProfile);
                                toleranced.id = result.id;
                                toleranced.type = result.type;
                                toleranced.units = result.units;
                                toleranced.semiDiameter = result.semiDiameter;
                                toleranced.sampling = result.sampling;
                                toleranced.angle = result.angle;
                                toleranced.sagDataPoints = result.sagDataPoints;
                            };
                            m_profileService->startCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle, TaskSource::TolerancedSurfaceProfile);
                        }
                    }
                    ImGui::PopStyleColor(4);
                    ImGui::EndDisabled();
                }
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
