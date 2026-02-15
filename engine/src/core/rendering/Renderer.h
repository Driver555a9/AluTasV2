#pragma once

//glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
    };
}