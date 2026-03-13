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

        /// Vertex and Fragment must NOT be nullptr; geometry may be nullptr if not needed
        explicit Shader(const char* vertex, const char* fragment, const char* geometry, const ProvidedPointers meaning) noexcept;
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
        GLuint m_ID {0};

        void PrintCompilationErrors(GLuint shader, bool is_program) noexcept;
        void Delete() noexcept;
    };
}