#include <stdexcept>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include <windows.h>
#include "dde_client.h"
#include "gui.h"
#include <ctime>

void GuiManager::AddDebugLog(const char* msg) {
    if (ZemaxDDE::debug_log) {
        time_t now = time(0);
        char timestamp[26];
        ctime_s(timestamp, sizeof(timestamp), &now);
        timestamp[24] = '\0'; // Удаляем символ новой строки
        ZemaxDDE::debug_log->push_back(std::string(timestamp) + " " + msg);
    }
}

GuiManager::GuiManager(GLFWwindow* win, HWND hwndDDE) : window(win), hwndDDE(hwndDDE), dde_initialized(false), selectedMenuItem(0), surface_number(1), radius(0.0f), show_about_popup(false), show_features_popup(false), show_updates_popup(false), debug_log(nullptr) {
    errorMsg[0] = '\0';
    if (debug_log) AddDebugLog((std::string("GuiManager created with hwndDDE: ") + std::to_string((uintptr_t)hwndDDE)).c_str());
}

GuiManager::~GuiManager() {
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GuiManager::initialize() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui_ImplOpenGL3_CreateDeviceObjects();
}


void GuiManager::render() {
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
    float total_available_height = ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight();
    float sidebar_width = 200.0f;
    float sidebar_height = 250.0f;
    float debug_log_height = 100.0f;
    float content_height = 450.0f;

    ImGui::SetNextWindowPos(window_pos);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, total_available_height));

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
        try {
            if (!dde_initialized) {
                ZemaxDDE::initiateDDE(hwndDDE);
                dde_initialized = true;
                if (debug_log) AddDebugLog("DDE connection established successfully");
            } else {
                ZemaxDDE::terminateDDE();
                dde_initialized = false;
                if (debug_log) AddDebugLog("DDE connection terminated");
            }
        } catch (const std::runtime_error& e) {
            if (debug_log) AddDebugLog((std::string("DDE Error: ") + e.what()).c_str());
            setErrorMsg(e.what());
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

            ImGui::InputInt("Surface Number", &surface_number);
            if (surface_number < 1) surface_number = 1;

            if (ImGui::Button("Get Radius")) {
                try {
                    if (dde_initialized) {
                        if (surface_number <= 0) {
                            setErrorMsg("Invalid surface number");
                            return;
                        }
                        if (debug_log) AddDebugLog((std::string("Calling getSurfaceRadius with hwndDDE: ") + std::to_string((uintptr_t)hwndDDE)).c_str());
                        float radius = ZemaxDDE::getSurfaceRadius(hwndDDE, surface_number);
                        setRadius(radius);
                        if (debug_log) AddDebugLog(("Radius retrieved: " + std::to_string(radius)).c_str());
                    } else {
                        setErrorMsg("DDE not initialized");
                    }
                } catch (const std::runtime_error& e) {
                    if (debug_log) AddDebugLog((std::string("Error: ") + e.what()).c_str());
                    setErrorMsg(e.what());
                }
            }

            ImGui::Text("Radius of Surface %d: %.4f", surface_number, radius);
            if (errorMsg[0]) ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", errorMsg);

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

    ImGui::BeginChild("DebugLog", ImVec2(0, debug_log_height), true);
    if (ImGui::Button("Copy Debug Log")) {
        if (debug_log) {
            std::string log_text;
            for (const auto& log : *debug_log) {
                log_text += log + "\n";
            }
            ImGui::SetClipboardText(log_text.c_str());
            if (debug_log) AddDebugLog(("Debug log copied to clipboard at " + std::string(__TIME__)).c_str());
        }
    }
    if (debug_log) {
        for (const auto& log : *debug_log) {
            ImGui::TextUnformatted(log.c_str());
        }
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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiManager::setDDEStatus(bool initialized) {
    dde_initialized = initialized;
}

void GuiManager::setSelectedMenuItem(int item) {
    selectedMenuItem = item;
}

void GuiManager::setSurfaceNumber(int num) {
    surface_number = num;
}

void GuiManager::setRadius(float r) {
    radius = r;
}

void GuiManager::setErrorMsg(const char* msg) {
    strncpy_s(errorMsg, msg, sizeof(errorMsg) - 1);
    errorMsg[sizeof(errorMsg) - 1] = '\0';
}

bool GuiManager::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void GuiManager::setDebugLog(std::vector<std::string>& log) {
    debug_log = &log;
}
