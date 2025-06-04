#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <windows.h>
#include "zemax_dde.h"
#include <string>

// Декларация внешней функции
extern LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

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

    // Загрузка шрифта
    io.Fonts->Clear(); // Очищаем стандартный шрифт ImGui
    ImFont* mainFont = AddSystemFont(17.0f); // Загружаем Segoe UI

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Пересоздаём текстуры шрифта
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Создание окна для DDE
    WNDCLASSEXA wndclass = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "ZEMAX_DDE_Client", NULL };
    RegisterClassExA(&wndclass);
    HWND hwndDDE = CreateWindowA("ZEMAX_DDE_Client", "DDE Client", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hwndDDE || !initialize_dde(hwndDDE)) {
        MessageBoxA(NULL, "Failed to initialize DDE", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(hwndDDE);
        glfwTerminate();
        return -1;
    }

    // Запуск потока для обработки сообщений
    HANDLE hThread = CreateThread(NULL, 0, DDEMessageThread, hwndDDE, 0, NULL);
    if (!hThread) {
        MessageBoxA(NULL, "Failed to create DDE thread", "Error", MB_OK | MB_ICONERROR);
        DestroyWindow(hwndDDE);
        glfwTerminate();
        return -1;
    }

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

        ImGui::BeginChild("Sidebar", ImVec2(200, window_size.y - 20), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        if (ImGui::Button("Optical system information", ImVec2(-1, 0))) selectedMenuItem = 0;
        if (ImGui::Button("Menu Item 2", ImVec2(-1, 0))) selectedMenuItem = 1;
        if (ImGui::Button("Menu Item 3", ImVec2(-1, 0))) selectedMenuItem = 2;
        
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::SameLine();

        // Основное содержимое
        ImGui::BeginChild("Content", ImVec2(0, -1), true);
        
        // Отображаем контент в зависимости от выбранного пункта
        switch (selectedMenuItem) {
            case 0:
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 10));
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "OPTICAL SYSTEM INFORMATION");
                ImGui::PopStyleVar();
                ImGui::Spacing();

                ImGui::TextWrapped("Get Radius of Surface");
                ImGui::Spacing();
                
                static char surface[128] = "1"; // Поле для ввода номера поверхности
                static std::string result = "Enter surface number and click Get Radius"; // Результат
                
                ImGui::Text("Surface Number:");
                ImGui::InputText("##Surface", surface, IM_ARRAYSIZE(surface), ImGuiInputTextFlags_CharsDecimal);
                
                if (ImGui::Button("Get Radius")) {
                    const char* radius_str = send_zemax_request(surface);
                    result = radius_str ? radius_str : "No data";
                }
                
                ImGui::Text("Radius: %s", result.c_str());
                break;
                
            case 1:
                ImGui::TextWrapped("Content for Menu Item 2");
                ImGui::Spacing();
                static float slider_value = 0.5f;
                ImGui::SliderFloat("Parameter", &slider_value, 0.0f, 1.0f);
                break;
                
            case 2:
                ImGui::TextWrapped("Content for Menu Item 3");
                ImGui::Spacing();
                static bool checkbox_value = true;
                ImGui::Checkbox("Enable feature", &checkbox_value);
                break;
        }
        
        ImGui::EndChild();
        ImGui::End();

        // Всплывающие окна
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
