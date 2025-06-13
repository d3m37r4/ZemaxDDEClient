#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include <string>
#include <vector>
#include <ctime>
#include "zemax_dde.h"

extern LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

std::vector<std::string> debug_log;
void AddDebugLog(const char* message) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%y - %H:%M:%S", timeinfo);
    debug_log.push_back(std::string(timestamp) + ": " + message);
}

ImFont* AddSystemFont(float size_pixels) {
    ImGuiIO& io = ImGui::GetIO();
    char fontPath[MAX_PATH];
    GetWindowsDirectoryA(fontPath, MAX_PATH);
    strcat_s(fontPath, "\\Fonts\\segoeui.ttf");
    return io.Fonts->AddFontFromFileTTF(fontPath, size_pixels);
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "ZemaxDDEClient", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    AddSystemFont(17.0f);

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

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
            if (ImGui::Button("Quit", ImVec2(50, 0))) glfwSetWindowShouldClose(window, true);
            ImGui::EndMainMenuBar();
        }

        ImVec2 window_pos = ImVec2(0, ImGui::GetFrameHeight());
        float total_available_height = io.DisplaySize.y - ImGui::GetFrameHeight();
        float sidebar_width = 200.0f;
        float sidebar_height = 250.0f;
        float debug_log_height = 100.0f;
        float content_height = 450.0f;

        ImGui::SetNextWindowPos(window_pos);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, total_available_height));

        ImGui::Begin("Main Content", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, sidebar_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        ImGui::BeginChild("DDE Status Frame", ImVec2(sidebar_width, 60), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("DDE Status:");
        ImGui::SameLine();

        char status_text[32];
        strncpy_s(status_text, dde_initialized ? "Initialized" : "Not Initialized", sizeof(status_text));
        ImGui::PushStyleColor(ImGuiCol_Text, dde_initialized ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##DDEStatus", status_text, sizeof(status_text), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
        float button_height = ImGui::GetFrameHeight();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.0f));
        if (ImGui::Button(dde_initialized ? "Close DDE" : "Init DDE", ImVec2(-1, button_height))) {
            AddDebugLog(dde_initialized ? "Attempting to close DDE..." : "Attempting to initialize DDE...");
            if (!dde_initialized) {
                if (!hwndDDE) {
                    WNDCLASSEXA wndclass = { 
                        sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, 
                        GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "ZEMAX_DDE_Client", NULL 
                    };
                    RegisterClassExA(&wndclass);
                    hwndDDE = CreateWindowExA(0, "ZEMAX_DDE_Client", "DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
                    if (!hwndDDE) {
                        AddDebugLog("Failed to create DDE window");
                        MessageBoxA(NULL, "Failed to create DDE window", "Error", MB_OK | MB_ICONERROR);
                        ImGui::PopStyleColor(3);
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                        ImGui::EndChild();
                        continue;
                    }
                    AddDebugLog("DDE window created");
                    // Запускаем поток обработки сообщений
                    hThread = CreateThread(NULL, 0, DDEMessageThread, hwndDDE, 0, NULL);
                    if (!hThread) {
                        AddDebugLog("Failed to create DDE message thread");
                        DestroyWindow(hwndDDE);
                        hwndDDE = NULL;
                        continue;
                    }
                }
                bool init_success = false;
                for (int attempt = 0; attempt < 2 && !init_success; ++attempt) {
                    if (initialize_dde(hwndDDE)) {
                        init_success = true;
                        dde_initialized = true;
                        AddDebugLog("DDE initialized successfully.");
                    } else {
                        std::string msg = "DDE initialization attempt " + std::to_string(attempt + 1) + " failed.";
                        AddDebugLog(msg.c_str()); // Используем .c_str() для преобразования
                        Sleep(100);
                    }
                }
                if (!init_success) {
                    AddDebugLog("Failed to initialize DDE after retries.");
                }
            } else {
                close_dde(hwndDDE);
                dde_initialized = false;
                AddDebugLog("DDE connection closed.");
                if (hThread) {
                    PostThreadMessage(GetThreadId(hThread), WM_QUIT, 0, 0);
                    WaitForSingleObject(hThread, 1000);
                    CloseHandle(hThread);
                    hThread = NULL;
                }
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild();
        ImGui::Spacing();

        if (ImGui::Button("Optical system information", ImVec2(-1, 0))) selectedMenuItem = 0;
        if (ImGui::Button("Local error analysis\nfor aspherical surface", ImVec2(-1, 0))) selectedMenuItem = 1;

        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Content", ImVec2(0, content_height), true);
        switch (selectedMenuItem) {
            case 0: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
                ImGui::PopStyleVar();
                ImGui::Spacing();
                ImGui::Text("text 1");
                break;
            }
            case 1: {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "LOCAL ERROR ANALYSIS FOR ASPHERICAL SURFACE");
                ImGui::PopStyleVar();
                ImGui::Spacing();
                ImGui::Text("text 2");
                break;
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // Debug
        ImGui::BeginChild("DebugLog", ImVec2(0, debug_log_height), true);
        if (ImGui::Button("Copy Debug Log")) {
            std::string log_text;
            for (const auto& log : debug_log) {
                log_text += log + "\n";
            }
            ImGui::SetClipboardText(log_text.c_str());
            AddDebugLog("Debug log copied to clipboard.");
        }
        for (const auto& log : debug_log) {
            ImGui::TextUnformatted(log.c_str());
        }
        ImGui::EndChild();

        ImGui::End();

        if (show_about_popup) { ImGui::OpenPopup("About"); show_about_popup = false; }
        if (show_features_popup) { ImGui::OpenPopup("Software Features"); show_features_popup = false; }
        if (show_updates_popup) { ImGui::OpenPopup("Check for Updates"); show_updates_popup = false; }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("ZemaxDDEClient\nVersion 1.0\n\n(c) 2023 Your Company");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Software Features", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Software Features:");
            ImGui::BulletText("Feature 1");
            ImGui::BulletText("Feature 2");
            ImGui::BulletText("Feature 3");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Check for Updates", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Your software is up to date!");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (hwndDDE) {
        PostMessage(hwndDDE, WM_DESTROY, 0, 0);
        if (hThread) {
            WaitForSingleObject(hThread, 1000);
            CloseHandle(hThread);
        }
        DestroyWindow(hwndDDE);
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
