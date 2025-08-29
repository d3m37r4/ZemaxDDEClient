#include <string>
#include <windows.h>
#include <nfd.h>
#include "lib/imgui/imgui.h"
#include "gui/components/gui_menu_bar.h"
#include "gui/gui.h"

namespace gui {
    void GuiManager::renderMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Open .zmx in Zemax")) {
                    nfdchar_t* outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog("zmx", nullptr, &outPath);

                    if (result == NFD_OKAY) {
                        std::string filePath = std::string(outPath);
                        printf("Selected: %s\n", filePath.c_str());

                        int size = MultiByteToWideChar(CP_UTF8, 0, outPath, -1, nullptr, 0);
                        std::wstring widePath(size, L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, outPath, -1, widePath.data(), size);
                        ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOW);

                        free(outPath);
                    } else if (result == NFD_CANCEL) {
                        printf("User cancelled.\n");
                    } else {
                        printf("Error: %s\n", NFD_GetError());
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) glfwSetWindowShouldClose(glfwWindow, true);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                if (ImGui::MenuItem("Check for updates")) show_updates_popup = true;
                if (ImGui::MenuItem("About")) show_about_popup = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}
