#include <stdexcept>
#include <string>
#include "lib/imgui/imgui.h"
#include "dde/dde_zemax_client.h"
#include "gui/content_pages/gui_page_optical_system_info.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderPageOpticalSystemInfo() {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
        ImGui::Spacing();
        if (dde_initialized) {
            ImGui::Text("Information about current system");
            ImGui::Spacing();

            const ZemaxDDE::OpticalSystemData& opticalSystem = zemaxDDEClient->getOpticalSystemData(); 
            ImGui::Text("Lens Name: %s", opticalSystem.lensName.c_str());
            ImGui::Text("File Name: %s", opticalSystem.fileName.c_str());
            ImGui::Spacing();

            ImGui::Text("Number of Surfaces: %d", opticalSystem.numSurfs);
            ImGui::Text("Stop Surface: %d", opticalSystem.stopSurf);
            ImGui::Text("Global Reference Surface: %d", opticalSystem.globalRefSurf);
            ImGui::Spacing();

            ImGui::Text("Non-Axial Flag: %s", opticalSystem.nonAxialFlag ? "Non-Axial" : "Axial");
            ImGui::Text("Ray Aiming Type: %s", gui::getRayAimingTypeString(opticalSystem.rayAimingType));
            ImGui::Text("Adjust Index: %s", opticalSystem.adjustIndex ? "True" : "False");
            ImGui::Spacing();

            ImGui::Text("Units: %s", getUnitString(opticalSystem.units));
            ImGui::Text("Temperature: %.4f °C", opticalSystem.temp);
            ImGui::Text("Pressure: %.4f atm", opticalSystem.pressure);
            ImGui::Spacing();

            ImGui::Text("Number of Fields: %d (Type %d)", opticalSystem.numFields, opticalSystem.fieldType);
            if (ImGui::CollapsingHeader("Field data")) {
                if (ImGui::BeginTable("FieldData", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("#");
                    ImGui::TableSetupColumn("X-Field");
                    ImGui::TableSetupColumn("Y-Field");
                    ImGui::TableHeadersRow();
                    for (int i = 1; i <= opticalSystem.numFields; i++) {
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
        } else {
            ImGui::Text("To get data, initialize a DDE connection with Zemax server.");
        }
    }
}
