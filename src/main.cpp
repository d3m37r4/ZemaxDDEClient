#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "zemax_dde.h"
#include <string>
#include <vector>

extern LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

std::vector<std::string> debug_log;
void AddDebugLog(const char* message) {
    debug_log.push_back(std::string(message));
}

ImFont* AddSystemFont(float size_pixels) {
    ImGuiIO& io = ImGui::GetIO();
    
    char fontPath[MAX_PATH];
    GetWindowsDirectoryA(fontPath, MAX_PATH);
    strcat_s(fontPath, "\\Fonts\\segoeui.ttf");

    return io.Fonts->AddFontFromFileTTF(fontPath, size_pixels);
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "ZemaxDDEClient", NULL, NULL);
    if (!window) { 
        glfwTerminate(); 
        return -1; 
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Load Font
    io.Fonts->Clear(); // Clear standart fonts ImGui
    ImFont* mainFont = AddSystemFont(17.0f); // Load Segoe UI

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Font
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    // For DDE
    HWND hwndDDE = NULL;
    HANDLE hThread = NULL;
    bool dde_initialized = false;
    int selectedMenuItem = 0;
    bool show_about_popup = false;
    bool show_features_popup = false;
    bool show_updates_popup = false;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                ImGui::MenuItem("Menu item 1");
                ImGui::MenuItem("Menu item 2");
                ImGui::MenuItem("Menu item 3");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                if (ImGui::MenuItem("Software features")) show_features_popup = true;
                if (ImGui::MenuItem("Check for updates")) show_updates_popup = true;
                if (ImGui::MenuItem("About")) show_about_popup = true;
                ImGui::EndMenu();
            }
            ImGui::SameLine(ImGui::GetWindowWidth() - 60);
            if (ImGui::Button("Quit", ImVec2(50, 0))) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMainMenuBar();
        }

        // Main content
        ImVec2 window_pos = ImVec2(0, ImGui::GetFrameHeight());
        ImVec2 window_size = ImVec2(io.DisplaySize.x, io.DisplaySize.y - ImGui::GetFrameHeight());
        ImGui::SetNextWindowPos(window_pos);
        ImGui::SetNextWindowSize(window_size);

        ImGui::Begin("Main Content", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Sidebar (Left) noborder, width 200 px
        ImGui::BeginChild("Sidebar", ImVec2(200, window_size.y), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        ImGui::BeginChild("DDE Status Frame", ImVec2(200, 60), true, ImGuiWindowFlags_NoScrollbar); // Height 60px
        ImGui::Text("DDE Status:");
        ImGui::SameLine();

        // Status
        char status_text[32];
        strncpy_s(status_text, dde_initialized ? "Initialized" : "Not Initialized", sizeof(status_text));
        ImGui::PushStyleColor(ImGuiCol_Text, dde_initialized ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Green or red?
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##DDEStatus", status_text, sizeof(status_text), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor();

        // Button 'Init DDE'
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        float button_height = ImGui::GetFrameHeight();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.0f));
        if (ImGui::Button("Init DDE", ImVec2(-1, button_height))) {
            debug_log.push_back("Attempting to initialize DDE...");
            if (!hwndDDE) {
                WNDCLASSEXA wndclass = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "ZEMAX_DDE_Client", NULL };
                RegisterClassExA(&wndclass);
                hwndDDE = CreateWindowExA(0, "ZEMAX_DDE_Client", "DDE Client", 0, 0, 0, 0, 0, 
                                        HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
                if (!hwndDDE) {
                    debug_log.push_back("Failed to create DDE window");
                    MessageBoxA(NULL, "Failed to create DDE window", "Error", MB_OK | MB_ICONERROR);
                    ImGui::PopStyleVar(3);
                    ImGui::EndChild();
                    ImGui::EndChild();
                    continue;
                }
                debug_log.push_back("DDE window created");
            }
            // Удаляем создание потока, так как оно не используется корректно без DDEMessageThread
            if (initialize_dde(hwndDDE)) {
                dde_initialized = true;
                debug_log.push_back("DDE initialized successfully.");
            } else {
                dde_initialized = false;
                debug_log.push_back("Failed to initialize DDE.");
            }
        }
        ImGui::PopStyleColor(3); // Reset colors of button (Button, ButtonHovered, ButtonActive)
        ImGui::PopStyleVar(); // Init DDE

        ImGui::EndChild(); // Конец блока в рамке

        ImGui::Spacing(); // Небольшой отступ перед кнопками

        if (ImGui::Button("Optical system information", ImVec2(-1, 0))) selectedMenuItem = 0;
        if (ImGui::Button("Local error analysis \nof aspherical surface", ImVec2(-1, 0))) selectedMenuItem = 1;
        
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::SameLine();

        // Основное содержимое
        ImGui::BeginChild("Content", ImVec2(0, -1), true);
        
        // Отображаем контент в зависимости от выбранного пункта
        switch (selectedMenuItem) {
            case 0: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
                ImGui::PopStyleVar();
                ImGui::Spacing();

                if (dde_initialized) {
                    SystemData* sys_data = GetSystemData();
                    ImGui::Text("Lens Name: %s", sys_data->lensname);
                    ImGui::Text("File Name: %s", sys_data->filename);
                    ImGui::Text("Number of Surfaces: %d", sys_data->numsurfs);
                    const char* units_str[] = {"mm", "cm", "in", "Meters"};
                    ImGui::Text("Units: %s", units_str[sys_data->units]);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextWrapped("Get Radius of Surface");
                    ImGui::Spacing();

                    static char surface[128] = "1";
                    static std::string result = "Enter surface number and click Get Radius";

                    ImGui::Text("Surface Number:");
                    ImGui::InputText("##Surface", surface, IM_ARRAYSIZE(surface), ImGuiInputTextFlags_CharsDecimal);
                    
                    if (ImGui::Button("Get Radius")) {
                        debug_log.push_back("Sending radius request for surface: " + std::string(surface));
                        const char* radius_str = send_zemax_request(surface);
                        result = radius_str ? radius_str : "No data";
                        debug_log.push_back("Radius result: " + result);
                    }
                   
                    ImGui::Text("Radius: %s", result.c_str());
                } else {
                    ImGui::Text("Please initialize DDE first.");
                }

                ImGui::Spacing();
                ImGui::Separator();
                
                ImGui::Text("Debug Log:");
                // Кнопка для копирования дебаг-лога
                if (ImGui::Button("Copy Debug Log")) {
                    std::string log_text;
                    for (const auto& log : debug_log) {
                        log_text += log + "\n";
                    }
                    ImGui::SetClipboardText(log_text.c_str());
                    AddDebugLog("Debug log copied to clipboard.");
                }
                ImGui::BeginChild("DebugLog", ImVec2(0, 100), true);
                for (const auto& log : debug_log) {
                    ImGui::Text("%s", log.c_str());
                }
                ImGui::EndChild();
                break;
            }    
            case 1: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL ERROR ANALYSIS FOR ASPHERICAL SURFACE");
                ImGui::PopStyleVar();
                ImGui::Spacing();

                ImGui::Text("text");
                break;
            }
        }
        
        ImGui::EndChild();
        ImGui::End();

        if (show_about_popup) {
            ImGui::OpenPopup("About");
            show_about_popup = false;
        }
        if (show_features_popup) {
            ImGui::OpenPopup("Software Features");
            show_features_popup = false;
        }
        if (show_updates_popup) {
            ImGui::OpenPopup("Check for Updates");
            show_updates_popup = false;
        }

        // Центрирование всплывающих окон
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // Окно "About"
        if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("ZemaxDDEClient\nVersion 1.0\n\n(c) 2023 Your Company");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Окно "Software Features"
        if (ImGui::BeginPopupModal("Software Features", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Software Features:");
            ImGui::BulletText("Feature 1");
            ImGui::BulletText("Feature 2");
            ImGui::BulletText("Feature 3");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Окно "Check for Updates"
        if (ImGui::BeginPopupModal("Check for Updates", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Your software is up to date!");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Рендеринг
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Очистка
    if (hwndDDE) {
        PostMessage(hwndDDE, WM_DESTROY, 0, 0);
        WaitForSingleObject(hThread, 1000);
        CloseHandle(hThread);
        DestroyWindow(hwndDDE);
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
