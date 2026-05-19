#include <algorithm>
#include <format>

#include "gui/gui.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"

namespace {
    std::string getSamplingTooltip() {
        return std::format(
            "Number of points to sample along the surface diameter (min={}, max={}).\n"
            "Higher values = smoother profile, slower calculation.",
            gui::MIN_SAMPLING, gui::MAX_SAMPLING
        );
    }

    std::string getAngleStepTooltip() {
        return "Angular step between cross sections in degrees (0-180).\n"
               "Smaller step = more detailed map, slower calculation.";
    }

    std::string getAngleTooltip() {
        return "Orientation angle in degrees relative to the local x axis.";
    }
}

namespace gui {
    void GuiManager::renderSurfaceMapAnalysis() {
        static int nominalSurfaceIndex = 0;
        static int nominalSampling = 128;
        static double nominalAngle = 0.0;

        static int tolerancedSurfaceIndex = 0;
        static int tolerancedSampling = 128;
        static double tolerancedAngleStep = 1.0;

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
        ImGui::InputInt("##nominal_surf_num", &nominalSurfaceIndex, 1, 10);
        nominalSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, nominalSurfaceIndex));

        ImGui::Text("Sampling:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##nominal_sampling", &nominalSampling, 10, 50);
        nominalSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, nominalSampling));
        ImGui::SameLine();
        HelpMarker(getSamplingTooltip().c_str());

        ImGui::Text("Angle:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputDouble("##nominal_angle", &nominalAngle, 1.0, 10.0, "%.2f");
        nominalAngle = std::clamp(nominalAngle, -360.0, 360.0);
        ImGui::SameLine();
        HelpMarker(getAngleTooltip().c_str());

        if (ImGui::Button("Get nominal surface data")) {
            if (isDDEInitialized()) {
                m_zemaxDDEClient->setStorageTarget(m_zemaxDDEClient->getNominalSurface());
                m_zemaxDDEClient->getSurfaceData(nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
                m_zemaxDDEClient->getSurfaceData(nominalSurfaceIndex, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);
                m_sagService->calculateSagCrossSection(nominalSurfaceIndex, nominalSampling, nominalAngle);
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
        ImGui::InputInt("##toleranced_surf_num", &tolerancedSurfaceIndex, 1, 10);
        tolerancedSurfaceIndex = std::max(0, std::min(m_zemaxDDEClient->getOpticalSystemData().numSurfs, tolerancedSurfaceIndex));

        ImGui::Text("Sampling:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputInt("##toleranced_sampling", &tolerancedSampling, 10, 50);
        tolerancedSampling = std::max(gui::MIN_SAMPLING, std::min(gui::MAX_SAMPLING, tolerancedSampling));
        ImGui::SameLine();
        HelpMarker(getSamplingTooltip().c_str());

        ImGui::Text("Angle step:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
        ImGui::InputDouble("##toleranced_angle_step", &tolerancedAngleStep, 0.1, 1.0, "%.2f");
        tolerancedAngleStep = std::clamp(tolerancedAngleStep, 0.01, 180.0);
        ImGui::SameLine();
        HelpMarker(getAngleStepTooltip().c_str());

        ImGui::EndDisabled();

        ImGui::EndChild();

        ImGui::SeparatorText("Analysis");

        ImGui::BeginDisabled(!isDDEInitialized());
        if (ImGui::Button("Calculate Surface Map")) {
            // TODO: implement calculation
        }
        ImGui::EndDisabled();
    }
}
