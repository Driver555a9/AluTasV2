#include "core/utility/DebugUtility.h"
#include "core/utility/ColorCodes.h"

#include "core/application/ApplicationGlobalState.h"

//std
#include <iostream>

//Glfw
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

//Win
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif 

namespace CoreEngine::DebugUtility
{
    void ForceCloseConsole() 
    {
    #ifdef _WIN32
        fclose(stdout);
        fclose(stderr);
        FreeConsole();
    #endif
    }

#ifdef _WIN32
namespace 
{
    BOOL WINAPI ConsoleHandler(DWORD ctrlType)
    {
        switch (ctrlType)
        {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_CLOSE_EVENT: 
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                GlobalSet<GlobalSet_StopApplication>();
        }
        return FALSE;
    }
}
#endif

    void ForceInitConsole()
    {
    #ifdef _WIN32
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
        {
            mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    #endif
    }

    void GLAPIENTRY OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        if (severity != GL_DEBUG_SEVERITY_HIGH) return;
        std::cerr << ColorCodes::RED << "OpenGL Debug Message (" << id << "): " << ColorCodes::RESET << message << std::endl;
    }

    void GlfwErrorMessageCallback(int error_code, const char* description)
    {
        std::cerr << ColorCodes::RED << "Glfw error message (" << error_code << ") " << ColorCodes::RESET << description << std::endl;
    }

    void EnableDebugMessages() 
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(OpenGLDebugMessageCallback, nullptr);
        glfwSetErrorCallback(GlfwErrorMessageCallback);
    }

    void PrintHardwareInfo() 
    {
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

        // Limits on Shader Storage Buffer Objects (SSBOs)
        GLint maxSSBOsPerStage, maxSSBOsTotal, maxSSBOSize;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOsTotal);
        glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxSSBOsPerStage);
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);
        std::cout << "Max SSBO Bindings: " << maxSSBOsTotal << std::endl;
        std::cout << "Max SSBO per Shader Stage: " << maxSSBOsPerStage << std::endl;
        std::cout << "Max SSBO Size: " << maxSSBOSize / (1024.0 * 1024.0) << " MB" << std::endl;

        // Limits on Uniform Buffer Objects (UBOs)
        GLint maxUBOs, maxUBOSize;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUBOs);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
        std::cout << "Max UBO Bindings: " << maxUBOs << std::endl;
        std::cout << "Max UBO Size: " << maxUBOSize / 1024.0 << " KB" << std::endl;

        // Maximum number of Texture Units
        GLint maxTextureUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
        std::cout << "Max Texture Units: " << maxTextureUnits << std::endl;

        // Maximum number of Vertex Attributes
        GLint maxVertexAttribs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
        std::cout << "Max Vertex Attributes: " << maxVertexAttribs << std::endl;

        // Maximum Elements in Indices and Vertices
        GLint maxElementsIndices, maxElementsVertices;
        glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxElementsIndices);
        glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxElementsVertices);
        std::cout << "Max Elements Indices: " << maxElementsIndices << std::endl;
        std::cout << "Max Elements Vertices: " << maxElementsVertices << std::endl;

        // Maximum Draw Buffers
        GLint maxDrawBuffers;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
        std::cout << "Max Draw Buffers: " << maxDrawBuffers << std::endl;

        // Maximum Uniform Locations
        GLint maxUniformLocations;
        glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformLocations);
        std::cout << "Max Uniform Locations: " << maxUniformLocations << std::endl;
    }
}