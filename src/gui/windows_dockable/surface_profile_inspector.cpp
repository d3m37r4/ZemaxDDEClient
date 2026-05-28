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
            "Total number of sample points across the full surface diameter (min={}, max={}).\n"
            "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
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
                ImGui::TextUnformatted("Optical system:");
                ImGui::SameLine();
                ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::TextUnformatted("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##nominal_surf_num", &state.nominalSurfaceIndex, 1, 10);
            state.nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0, state.nominalSurfaceIndex));

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

            if (ImGui::Button("Copy toleranced surface settings")) {
                state.nominalSampling = state.tolerancedSampling;
                state.nominalAngle = state.tolerancedAngle;
            }

            ImGui::SameLine();

            if (m_uiOpMonitor.isActive(TaskSource::NominalSurfaceProfile)) {
                ImGuiUtils::SpinnerButton("Processing...", true);
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_profileService->cancelCalculation();
                    m_profileService->onCalculationComplete = nullptr;
                    nominal.clear();
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
                        m_profileService->startAsyncSagCalculation(state.nominalSurfaceIndex, state.nominalSampling, state.nominalAngle, TaskSource::NominalSurfaceProfile);
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

            if (ImGui::Button("Export txt")) {
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
                ImGui::TextUnformatted("Optical system:");
                ImGui::SameLine();
                ImGui::InputText("##optical_system", fileName.data(), fileName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::TextUnformatted("Surface number:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##toleranced_surf_num", &state.tolerancedSurfaceIndex, 1, 10);
            state.tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient ? m_zemaxDDEClient->getOpticalSystemData().numSurfs : 0, state.tolerancedSurfaceIndex));

            ImGui::TextUnformatted("Sampling:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
            ImGui::InputInt("##toleranced_sampling", &state.tolerancedSampling, 10, 50);
            state.tolerancedSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, state.tolerancedSampling)) | 1;
            ImGui::SameLine();
            ImGuiUtils::HelpMarker(getSamplingTooltip().c_str());

            ImGui::TextUnformatted("Angle:");
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

            if (m_uiOpMonitor.isActive(TaskSource::TolerancedSurfaceProfile)) {
                ImGuiUtils::SpinnerButton("Processing...", true);
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_profileService->cancelCalculation();
                    m_profileService->onCalculationComplete = nullptr;
                    toleranced.clear();
                }
            } else if (m_uiOpMonitor.hasActiveTasks()) {
                ImGui::BeginDisabled(true);
                ImGui::Button("Get toleranced surface data");
                if (ImGui::BeginItemTooltip()) {
                    ImGui::TextUnformatted("Another calculation is in progress");
                    ImGui::EndTooltip();
                }
                ImGui::EndDisabled();
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
                        m_profileService->startAsyncSagCalculation(state.tolerancedSurfaceIndex, state.tolerancedSampling, state.tolerancedAngle, TaskSource::TolerancedSurfaceProfile);
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
