#include <vector>
#include <string>
#include <algorithm>
#include <format>

#include "gui/gui.h"
#include "gui/constants.h"
#include "gui/surface_profile_service.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

namespace {
    std::string getSamplingTooltip() {
        return std::format(
            "Number of points per half-profile (from center to edge, min={}, max={}).\n"
            "Total points across full diameter = 2 × half − 1.\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderSurfaceProfileInspector() {
        auto& state = m_profileService->m_pageState;
        auto& nominal = m_profileService->m_nominalSurfaceData;
        auto& toleranced = m_profileService->m_tolerancedSurfaceData;

        ImGui::BeginChild("##SurfaceProfileContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        ImGui::SeparatorText("Nominal surface parameters");
        ImGui::BeginChild(
            "NominalSurfaceContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY,
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
                m_profileService->saveCrossSectionToFile(nominal);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_profileService->m_showNominalProfileWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_profileService->m_showNominalProfileWindow = false;
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
            ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());

            ImGui::Text("Angle:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputDouble("##nominal_angle", &state.nominalAngle, 1.0, 10.0, "%.2f");
            state.nominalAngle = std::clamp(state.nominalAngle, -360.0, 360.0);
            ImGui::SameLine();
            ImGuiUtils::HelpMarker(getAngleTooltip().c_str());

            ImGuiUtils::SpacingY(0.5f);

            if (ImGui::Button("Copy toleranced surface settings")) {
                state.nominalSampling = state.tolerancedSampling;
                state.nominalAngle = state.tolerancedAngle;
            }

            ImGui::SameLine();

            if (m_profileService->getCalcState() == SagCalcState::FetchingSurfaceData ||
                m_profileService->getCalcState() == SagCalcState::FetchingSagPoints) {
                float progress = m_profileService->getTotalSteps() > 0
                    ? static_cast<float>(m_profileService->getCurrentStep()) / m_profileService->getTotalSteps()
                    : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1, 0),
                    std::format("{}/{}", m_profileService->getCurrentStep(), m_profileService->getTotalSteps()).c_str());

                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_profileService->cancelCalculation();
                }

                if (m_profileService->getSkippedPoints() > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                        "%d points skipped (timeout)", m_profileService->getSkippedPoints());
                }
            } else {
                if (ImGui::Button("Get nominal surface data")) {
                    if (isDDEInitialized()) {
                        nominal.units = m_zemaxDDEClient->getOpticalSystemData().units;
                        nominal.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                        m_profileService->onCalculationComplete = [this, &nominal]() {
                            const auto& result = m_profileService->getResult();
                            nominal.id = result.id;
                            nominal.type = result.type;
                            nominal.units = result.units;
                            nominal.semiDiameter = result.semiDiameter;
                            nominal.sampling = result.sampling;
                            nominal.angle = result.angle;
                            nominal.sagDataPoints = result.sagDataPoints;
                        };
                        m_profileService->startAsyncSagCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle);
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
            ImGuiChildFlags_AutoResizeY,
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
                m_profileService->saveCrossSectionToFile(toleranced);
            }

            ImGui::SameLine();

            if (ImGui::Button("Show profile graphic")) {
                m_profileService->m_showTolerancedProfileWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear data")) {
                m_profileService->m_showTolerancedProfileWindow = false;
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
            ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());

            ImGui::Text("Angle:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputDouble("##toleranced_angle", &state.tolerancedAngle, 1.0, 10.0, "%.2f");
            state.tolerancedAngle = std::clamp(state.tolerancedAngle, -360.0, 360.0);
            ImGui::SameLine();
            ImGuiUtils::HelpMarker(getAngleTooltip().c_str());

            ImGuiUtils::SpacingY(0.5f);

            if (ImGui::Button("Copy nominal surface settings")) {
                state.tolerancedSampling = state.nominalSampling;
                state.tolerancedAngle = state.nominalAngle;
            }

            ImGui::SameLine();

            bool isCalculating = (m_profileService->getCalcState() == SagCalcState::FetchingSurfaceData ||
                                  m_profileService->getCalcState() == SagCalcState::FetchingSagPoints);

            if (isCalculating) {
                float progress = m_profileService->getTotalSteps() > 0
                    ? static_cast<float>(m_profileService->getCurrentStep()) / m_profileService->getTotalSteps()
                    : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1, 0),
                    std::format("{}/{}", m_profileService->getCurrentStep(), m_profileService->getTotalSteps()).c_str());

                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_profileService->cancelCalculation();
                }

                if (m_profileService->getSkippedPoints() > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                        "%d points skipped (timeout)", m_profileService->getSkippedPoints());
                }
            } else {
                if (ImGui::Button("Get toleranced surface data")) {
                    if (isDDEInitialized()) {
                        toleranced.units = m_zemaxDDEClient->getOpticalSystemData().units;
                        toleranced.fileName = m_zemaxDDEClient->getOpticalSystemData().fileName;

                        m_profileService->onCalculationComplete = [this, &toleranced]() {
                            const auto& result = m_profileService->getResult();
                            toleranced.id = result.id;
                            toleranced.type = result.type;
                            toleranced.units = result.units;
                            toleranced.semiDiameter = result.semiDiameter;
                            toleranced.sampling = result.sampling;
                            toleranced.angle = result.angle;
                            toleranced.sagDataPoints = result.sagDataPoints;
                        };
                        m_profileService->startAsyncSagCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle);
                    }
                }
            }

            ImGui::EndDisabled();
        }
        ImGui::EndChild();

        if (nominal.isValid() && toleranced.isValid()) {
            ImGui::SeparatorText("Surface profile analysis results");

            if (ImGui::Button("Show comparison graphic")) {
                m_profileService->m_showComparisonProfileWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Show deviation graphic")) {
                m_profileService->m_showDeviationProfileWindow = true;
            }
        }

        ImGui::EndChild();
    }
}
