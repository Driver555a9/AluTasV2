#include "core/rendering/Shader.h"

//std
#include <iostream>

//Own includes
#include "core/utility/CommonUtility.h"

#include "core/utility/Assert.h"

namespace CoreEngine
{
    Shader::Shader(const char* vertex, const char* fragment, const ProvidedPointers meaning)
    {
        const char* vertex_source_code;
        const char* frag_source_code;
        if (meaning == Shader::ProvidedPointers::ARE_FILE_PATHS)
        {
            vertex_source_code = CommonUtility::ReadFileToString(vertex).c_str();
            frag_source_code   = CommonUtility::ReadFileToString(fragment).c_str();
        }
        else if (meaning == Shader::ProvidedPointers::ARE_SOURCE_CODE)
        {
            vertex_source_code = vertex;
            frag_source_code   = fragment;
        }
        else 
        {
            ENGINE_ASSERT(false && ("At Shader::Shader() \"PointerMeaning\" is not recognizable: " + std::to_string(static_cast<int>(meaning))).c_str());
        }

        constexpr const bool IS_PROGRAM_LINK_ERR = true;

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_source_code, NULL);
        glCompileShader(vertex_shader);
        PrintCompilationErrors(vertex_shader, !IS_PROGRAM_LINK_ERR);

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &frag_source_code, NULL);
        glCompileShader(fragment_shader);
        PrintCompilationErrors(fragment_shader, !IS_PROGRAM_LINK_ERR);

        this->ID = glCreateProgram();
        glAttachShader(this->ID, vertex_shader);
        glAttachShader(this->ID, fragment_shader);
        glLinkProgram(this->ID);
        PrintCompilationErrors(ID, IS_PROGRAM_LINK_ERR);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    Shader::~Shader()
    {
        Delete();
    }

    void Shader::Activate()
    {
        glUseProgram(this->ID);
    }

    void Shader::Deactivate()
    {
        glUseProgram(0);
    }

    void Shader::Delete()
    {
        if(ID)
        {
            glDeleteProgram(this->ID);
        }
    }

    void Shader::PrintCompilationErrors(unsigned int shader, bool is_program)
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

    GLuint Shader::GetID() const
    {
        return ID;
    }
}