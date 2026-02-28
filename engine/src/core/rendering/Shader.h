#pragma once

#include <glad/gl.h>

namespace CoreEngine
{
    class Shader
    {
    public:
        enum class ProvidedPointers : int 
        {
            ARE_FILE_PATHS   = 0,
            ARE_SOURCE_CODE  = 1
        };

        explicit Shader(const char* vertex, const char* fragment, const ProvidedPointers meaning) noexcept;
        ~Shader() noexcept;

        Shader(Shader&&) noexcept;
        Shader& operator=(Shader&&) noexcept;

    //////////////////////////////////
    // Methods
    //////////////////////////////////
        void Activate() noexcept;
        void Deactivate() noexcept;

        [[nodiscard]] GLuint GetID() const noexcept;

    //////////////////////////////////
    // Copy/Move behaviour
    //////////////////////////////////
        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;
        
    private:
        GLuint ID {0};

        void PrintCompilationErrors(unsigned int shader, bool is_program) noexcept;
        void Delete() noexcept;
    };
}