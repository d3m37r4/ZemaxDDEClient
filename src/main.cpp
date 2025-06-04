#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

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
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

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

        // Сайдбар слева (200px шириной, фиксированной высоты без границ)
        ImGui::BeginChild("Sidebar", ImVec2(200, window_size.y - 20), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        // Стиль для кнопок сайдбара
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        if (ImGui::Button("Menu Item 1", ImVec2(-1, 0))) selectedMenuItem = 0;
        if (ImGui::Button("Menu Item 2", ImVec2(-1, 0))) selectedMenuItem = 1;
        if (ImGui::Button("Menu Item 3", ImVec2(-1, 0))) selectedMenuItem = 2;
        
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        // Вертикальный разделитель
        ImGui::SameLine();
        // ImGui::Separator();
        // ImGui::SameLine();

        // Основное содержимое
        ImGui::BeginChild("Content", ImVec2(0, -1), true);
        
        // Отображаем контент в зависимости от выбранного пункта
        switch (selectedMenuItem) {
            case 0:
                ImGui::TextWrapped("Content for Menu Item 1");
                ImGui::Spacing();
                if (ImGui::Button("Action Button")) {
                    // Действие при нажатии
                }
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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

