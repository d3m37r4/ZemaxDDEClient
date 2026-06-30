#include <format>

#include "gui/gui.h"
#include "gui/imgui_utils.h"
#include "lib/imgui/imgui.h"

namespace gui {
    void GuiManager::renderOpticalSystemInfo() {
        if (!isDDEInitialized()) {
            ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
            return;
        }

        const ZemaxDDE::OpticalSystemData& s = m_zemaxDDEClient->getOpticalSystemData();

        ImGui::BeginChild("##SystemInfoContent",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground
        );

        float cp = ImGui::GetStyle().CellPadding.x;
        float numW = ImGui::CalcTextSize("00").x + cp * 2.0f;
        float valW = ImGui::CalcTextSize("0000000000000000").x + cp * 2.0f;
        float blockW = numW + valW * 2.0f + cp * 2.0f;

        auto renderFieldTable = [&]() {
            if (ImGui::CollapsingHeader("Field data")) {
                if (ImGui::BeginTable("##FieldTable", 3,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, numW);
                    ImGui::TableSetupColumn("X-Field", ImGuiTableColumnFlags_WidthFixed, valW);
                    ImGui::TableSetupColumn("Y-Field", ImGuiTableColumnFlags_WidthFixed, valW);
                    ImGui::TableHeadersRow();
                    for (int i = ZemaxDDE::MIN_FIELDS; i <= s.numFields; i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(std::to_string(i).c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(std::format("{:g}", s.xField[i]).c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(std::format("{:g}", s.yField[i]).c_str());
                    }
                    ImGui::EndTable();
                }
            }
        };

        auto renderWaveTable = [&]() {
            if (ImGui::CollapsingHeader("Wave data")) {
                if (ImGui::BeginTable("##WaveTable", 3,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, numW);
                    ImGui::TableSetupColumn("Wavelength (μm)", ImGuiTableColumnFlags_WidthFixed, valW);
                    ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthFixed, valW);
                    ImGui::TableHeadersRow();
                    for (int i = ZemaxDDE::MIN_WAVES; i <= s.numWaves; i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(std::to_string(i).c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(std::format("{:g}", s.waveData[i].value).c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(std::format("{:g}", s.waveData[i].weight).c_str());
                    }
                    ImGui::EndTable();
                }
            }
        };

        const char* allLabels[] = {
            "Lens Name", "File Name", "Surfaces", "Units", "Stop Surface",
            "Global Reference Surface", "Fields", "Wavelengths", "Primary Wave",
            "Non-Axial", "Ray Aiming", "Adjust Index", "Temperature", "Pressure"
        };
        float maxLabelWidth = 0.0f;
        for (auto* l : allLabels)
            maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(l).x);
        maxLabelWidth += 8.0f;

        ImGuiUtils::SectionHeader("File");
        ImGuiUtils::BeginPropertyGrid("##File", maxLabelWidth);
        ImGuiUtils::PropertyGridRow("Lens Name", s.lensName.c_str());
        ImGuiUtils::PropertyGridRow("File Name", s.fileName.c_str());
        ImGuiUtils::EndPropertyGrid();
        ImGuiUtils::SpacingY(0.25f);

        ImGuiUtils::SectionHeader("System");
        ImGuiUtils::BeginPropertyGrid("##System", maxLabelWidth);
        ImGuiUtils::PropertyGridRow("Surfaces", std::to_string(s.numSurfs).c_str());
        ImGuiUtils::PropertyGridRow("Units", getUnitString(s.units));
        ImGuiUtils::PropertyGridRow("Stop Surface", std::to_string(s.stopSurf).c_str());
        ImGuiUtils::PropertyGridRow("Global Reference Surface", std::to_string(s.globalRefSurf).c_str());
        ImGuiUtils::EndPropertyGrid();
        ImGuiUtils::SpacingY(0.25f);

        ImGuiUtils::SectionHeader("Fields & Waves");
        ImGuiUtils::BeginPropertyGrid("##Fields", maxLabelWidth);
        ImGuiUtils::PropertyGridRow("Fields", std::format("{} (Type {})", s.numFields, s.fieldType).c_str());
        ImGuiUtils::PropertyGridRow("Wavelengths", std::to_string(s.numWaves).c_str());
        ImGuiUtils::PropertyGridRow("Primary Wave", std::to_string(s.primWave).c_str());
        ImGuiUtils::EndPropertyGrid();
        ImGuiUtils::SpacingY(0.25f);
        ImGui::BeginChild("##FieldWaveBlock", ImVec2(blockW, 0.0f), ImGuiChildFlags_AutoResizeY);
        renderFieldTable();
        renderWaveTable();
        ImGui::EndChild();
        ImGuiUtils::SpacingY(0.25f);

        ImGuiUtils::SectionHeader("Settings");
        ImGuiUtils::BeginPropertyGrid("##Settings", maxLabelWidth);
        ImGuiUtils::PropertyGridRow("Non-Axial", s.nonAxialFlag ? "Non-Axial" : "Axial");
        ImGuiUtils::PropertyGridRow("Ray Aiming", getRayAimingTypeString(s.rayAimingType));
        ImGuiUtils::PropertyGridRow("Adjust Index", s.adjustIndex ? "Yes" : "No");
        ImGuiUtils::EndPropertyGrid();
        ImGuiUtils::SpacingY(0.25f);

        ImGuiUtils::SectionHeader("Environment");
        ImGuiUtils::BeginPropertyGrid("##Env", maxLabelWidth);
        ImGuiUtils::PropertyGridRow("Temperature", std::format("{:.4f} °C", s.temp).c_str());
        ImGuiUtils::PropertyGridRow("Pressure", std::format("{:.4f} atm", s.pressure).c_str());
        ImGuiUtils::EndPropertyGrid();
        ImGuiUtils::SpacingY(0.25f);

        ImGui::EndChild();
    }
}
