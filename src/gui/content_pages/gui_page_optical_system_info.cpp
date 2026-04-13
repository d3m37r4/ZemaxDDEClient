#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageOpticalSystemInfo() {
        ImGui::Separator();
        ImGui::Spacing();
        if (isDdeInitialized()) {
            const ZemaxDDE::OpticalSystemData& opticalSystem = m_zemaxDDEClient->getOpticalSystemData();
            ImGui::TextUnformatted(std::format("Lens Name: {}", opticalSystem.lensName).c_str());
            ImGui::TextUnformatted(std::format("File Name: {}", opticalSystem.fileName).c_str());
            ImGui::TextUnformatted(std::format("There are {} surfaces", opticalSystem.numSurfs).c_str());
            ImGui::TextUnformatted(std::format("The units are {}", getUnitString(opticalSystem.units)).c_str());
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::TextUnformatted(std::format("Stop Surface: {}", opticalSystem.stopSurf).c_str());
            ImGui::TextUnformatted(std::format("Global Reference Surface: {}", opticalSystem.globalRefSurf).c_str());
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::TextUnformatted(std::format("Number of Fields: {} (Type {})", opticalSystem.numFields, opticalSystem.fieldType).c_str());
            ImGui::BeginChild("FieldDataContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY);
            if (ImGui::CollapsingHeader("Field data")) {
                if (ImGui::BeginTable("FieldData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("#");
                    ImGui::TableSetupColumn("X-Field");
                    ImGui::TableSetupColumn("Y-Field");
                    ImGui::TableHeadersRow();
                    for (int i = ZemaxDDE::MIN_FIELDS; i <= opticalSystem.numFields; i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(std::to_string(i).c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(std::format("{:.4f}", opticalSystem.xField[i]).c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(std::format("{:.4f}", opticalSystem.yField[i]).c_str());
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::TextUnformatted(std::format("There are {} wavelengths", opticalSystem.numWaves).c_str());
            ImGui::TextUnformatted(std::format("The primary wavelength is {}", opticalSystem.primWave).c_str());
            ImGui::BeginChild("WaveDataContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY);
            if (ImGui::CollapsingHeader("Wave data")) {
                if (ImGui::BeginTable("WaveDataTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("#");
                    ImGui::TableSetupColumn("Wavelength (μm)");
                    ImGui::TableSetupColumn("Weight");
                    ImGui::TableHeadersRow();
                    for (int i = ZemaxDDE::MIN_WAVES; i <= opticalSystem.numWaves; i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(std::to_string(i).c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(std::format("{:.4f}", opticalSystem.waveData[i].value).c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(std::format("{:.4f}", opticalSystem.waveData[i].weight).c_str());
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::TextUnformatted(std::format("Non-Axial Flag: {}", opticalSystem.nonAxialFlag ? "Non-Axial" : "Axial").c_str());
            ImGui::TextUnformatted(std::format("Ray Aiming Type: {}", gui::getRayAimingTypeString(opticalSystem.rayAimingType)).c_str());
            ImGui::TextUnformatted(std::format("Adjust Index: {}", opticalSystem.adjustIndex ? "True" : "False").c_str());
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::TextUnformatted(std::format("Temperature: {:.4f} \u00b0C", opticalSystem.temp).c_str());
            ImGui::TextUnformatted(std::format("Pressure: {:.4f} atm", opticalSystem.pressure).c_str());
        } else {
            ImGui::TextUnformatted("To get data, initialize a DDE connection with Zemax server.");
        }
    }
}
