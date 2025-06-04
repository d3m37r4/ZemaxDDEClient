#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <string>
#include <stdio.h>
#include "zemax_dde.h"

int main() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(400, 300, "Sag Data Client", NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    char surface[128] = "";
    char sampling[128] = "";
    char path[256] = "";
    bool running = true;
    std::string result;

    while (!glfwWindowShouldClose(window) && running) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Zemax DDE Client");
        ImGui::Text("Enter parameters:");
        ImGui::InputText("Surface", surface, IM_ARRAYSIZE(surface));
        ImGui::InputText("Sampling", sampling, IM_ARRAYSIZE(sampling));
        ImGui::InputText("Path", path, IM_ARRAYSIZE(path));

        if (ImGui::Button("Run")) {
            if (surface[0] && sampling[0] && path[0]) {
                result = send_z_surface(surface, sampling, path);
            } else {
                result = "Please fill all fields";
            }
        }
        if (ImGui::Button("Close")) {
            running = false;
        }
        ImGui::Text("Result: %s", result.c_str());
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
