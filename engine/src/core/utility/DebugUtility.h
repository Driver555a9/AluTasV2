#pragma once

#include <glad/gl.h>

namespace CoreEngine
{
    namespace DebugUtility
    {
        void ForceCloseConsole();

        void ForceInitConsole();

        void GLAPIENTRY OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

        void GlfwErrorMessageCallback(int error_code, const char* description);

        void EnableDebugMessages();

        void PrintHardwareInfo();
    }
}