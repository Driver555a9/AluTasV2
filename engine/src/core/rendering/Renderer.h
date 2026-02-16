#pragma once

//glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"

namespace CoreEngine
{
    class Renderer
    {
    public:
        Renderer() = delete;
    
        static void BeginFrame(GLFWwindow* window);
        static void FinalizeFrame(GLFWwindow* window);

        //////////////////////////////////////////////// 
        //--------- ImGui
        //////////////////////////////////////////////// 
        static void BeginImGuiFrame();
        static void FinalizeImGuiFrame();

        static void InitializeImGui(GLFWwindow* window);
        static void ShutdownImGui();

    private:
        friend class Application;
        //Values will be set in InitializeImGui()
        static inline ImGuiConfigFlags s_imgui_config_flags {};
    };
}