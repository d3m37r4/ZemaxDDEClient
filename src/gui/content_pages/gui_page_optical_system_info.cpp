#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageOpticalSystemInfo() {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (isDdeInitialized()) {
            const ZemaxDDE::OpticalSystemData& opticalSystem = zemaxDDEClient->getOpticalSystemData(); 
            ImGui::Text("Lens Name: %s", opticalSystem.lensName.c_str());
            ImGui::Text("File Name: %s", opticalSystem.fileName.c_str());
            ImGui::Text("There are %d surfaces", opticalSystem.numSurfs);
            ImGui::Text("The units are %s", getUnitString(opticalSystem.units));
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("Stop Surface: %d", opticalSystem.stopSurf);
            ImGui::Text("Global Reference Surface: %d", opticalSystem.globalRefSurf);
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("Number of Fields: %d (Type %d)", opticalSystem.numFields, opticalSystem.fieldType);
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
                        ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.4f", opticalSystem.xField[i]);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%.4f", opticalSystem.yField[i]);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("There are %d wavelengths", opticalSystem.numWaves);
            ImGui::Text("The primary wavelength is %d", opticalSystem.primWave);
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
                        ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.4f", opticalSystem.waveData[i].value);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%.4f", opticalSystem.waveData[i].weight);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("Non-Axial Flag: %s", opticalSystem.nonAxialFlag ? "Non-Axial" : "Axial");
            ImGui::Text("Ray Aiming Type: %s", gui::getRayAimingTypeString(opticalSystem.rayAimingType));
            ImGui::Text("Adjust Index: %s", opticalSystem.adjustIndex ? "True" : "False");
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            ImGui::Text("Temperature: %.4f °C", opticalSystem.temp);
            ImGui::Text("Pressure: %.4f atm", opticalSystem.pressure);
        } else {
            ImGui::Text("To get data, initialize a DDE connection with Zemax server.");
        }
    }
}
