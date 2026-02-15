#pragma once

#include <glad/gl.h>

namespace CoreEngine
{
    class Shader
    {
        private:
            GLuint ID {0};

            void PrintCompilationErrors(unsigned int shader, bool is_program);

            void Delete();

        public:
            enum class ProvidedPointers : int 
            {
                ARE_FILE_PATHS   = 0,
                ARE_SOURCE_CODE  = 1
            };

            explicit Shader(const char* vertex, const char* fragment, const ProvidedPointers meaning);
            ~Shader();

//--------------- Copy/Move behaviour
            Shader(const Shader&)            = delete;
            Shader& operator=(const Shader&) = delete;
            
            Shader(Shader&&)                 = default;
            Shader& operator=(Shader&&)      = default;
//---------------

            void Activate();
            void Deactivate();

            [[nodiscard]] GLuint GetID() const;
    };
}