#include "core/rendering/Shader.h"

//std
#include <iostream>

//Own includes
#include "core/utility/CommonUtility.h"

#include "core/utility/Assert.h"

namespace CoreEngine
{
    Shader::Shader(const char* vertex, const char* fragment, const char* geometry, const ProvidedPointers meaning) noexcept
    {
        ENGINE_ASSERT(vertex && fragment && "Vertex and fragment shader must not be nullptr; geometry shader may be nullptr.");
        std::string vertex_code_str;
        std::string frag_code_str;
        std::string geometry_code_str;

        const char* vertex_source_code   = nullptr;
        const char* frag_source_code     = nullptr;
        const char* geometry_source_code = nullptr;

        if (meaning == Shader::ProvidedPointers::ARE_FILE_PATHS)
        {
            try
            {
                vertex_code_str    = CommonUtility::ReadFileToString(vertex);
                vertex_source_code = vertex_code_str.c_str();

                frag_code_str     = CommonUtility::ReadFileToString(fragment); 
                frag_source_code  = frag_code_str.c_str();

                if (geometry)
                {
                    geometry_code_str    = CommonUtility::ReadFileToString(geometry);
                    geometry_source_code = geometry_code_str.c_str();
                }
            }
            catch (const std::exception& e)
            {
                ENGINE_ERROR_PRINT("Failed to create shader: " << e.what());
            }
        }
        else
        {
            vertex_source_code   = vertex;
            frag_source_code     = fragment;
            geometry_source_code = geometry;
        }

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_source_code, nullptr);
        glCompileShader(vs);
        PrintCompilationErrors(vs, false);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &frag_source_code, nullptr);
        glCompileShader(fs);
        PrintCompilationErrors(fs, false);

        GLuint gs = 0;
        if (geometry_source_code)
        {
            gs = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(gs, 1, &geometry_source_code, nullptr);
            glCompileShader(gs);
            PrintCompilationErrors(gs, false);
            glAttachShader(m_ID, gs);
        }

        m_ID = glCreateProgram();
        glAttachShader(m_ID, vs);
        glAttachShader(m_ID, fs);
        if (geometry_source_code) 
        {
            glAttachShader(m_ID, gs);
        }
        glLinkProgram(m_ID);
        PrintCompilationErrors(m_ID, true);

        glDeleteShader(vs);
        glDeleteShader(fs);
        if (geometry_source_code) 
        {
            glDeleteShader(gs);
        }
    }

    Shader::~Shader() noexcept
    {
        Delete();
    }

    Shader::Shader(Shader&& other) noexcept : m_ID(other.m_ID) 
    {
        other.m_ID = 0;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this != &other) 
        {
            Delete();
            m_ID       = other.m_ID;
            other.m_ID = 0;
        }
        return *this;
    }

    void Shader::Activate() noexcept
    {
        glUseProgram(this->m_ID);
    }

    void Shader::Deactivate() noexcept
    {
        glUseProgram(0);
    }

    void Shader::Delete() noexcept
    {
        if(m_ID)
        {
            glDeleteProgram(this->m_ID);
            m_ID = 0;
        }
    }

    void Shader::PrintCompilationErrors(GLuint shader, bool is_program) noexcept
    {
        GLint hasCompiled;
        char infoLog[1024];
        if (! is_program)
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "SHADER_COMPILATION_ERROR for: " << infoLog << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
            if (hasCompiled == GL_FALSE)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "SHADER_LINKING_ERROR for: " << infoLog << std::endl;
            }
        }
    }

    GLuint Shader::GetID() const noexcept
    {
        return m_ID;
    }
}