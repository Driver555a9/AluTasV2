#include "core/rendering/Renderer.h"

//glad
#include "glad/gl.h"

//ImGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "imgui/ImGuiFileDialog.h"

//Custom font
#include "default_fonts/JetBrainsMono.h"

//Standard imports
#include <iostream>

namespace CoreEngine 
{
    void Renderer::BeginFrame(GLFWwindow* window)
    {
        glfwMakeContextCurrent(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void Renderer::FinalizeFrame(GLFWwindow* window)
    {
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

//---------- ImGui
    void Renderer::BeginImGuiFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::FinalizeImGuiFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
    }

    void Renderer::InitializeImGui(GLFWwindow* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 460");

        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(JET_BRAINS_MONO_BOLD, JET_BRAINS_MONO_BOLD_length, 18.0f, &cfg);
        ImGui::StyleColorsDark();

        io.ConfigFlags |= s_imgui_config_flags;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        float scale = std::min(mode->width / 1920.0f, mode->height / 1080.0f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale);
        io.FontGlobalScale = scale;
    }

    void Renderer::ShutdownImGui()
    {
        ImGui::DestroyPlatformWindows();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();
    }
}