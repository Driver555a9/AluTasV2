#pragma once

//own includes
#include "core/model/Model.h"
#include "core/rendering/Texture.h"
#include "core/rendering/RenderBuffers.h"
#include "core/rendering/Shader.h"

namespace CoreEngine
{
    class FlatQuadImage_RenderPipeline
    {
        protected:
//-------------- Constexpr data
            constexpr static std::array<GLfloat, 24> s_DEFAULT_QUAD = {
                //Position x, y     Tex U, V
                -1.0f,  1.0f,    0.0f, 0.0f, //Right Bottom
                -1.0f, -1.0f,    0.0f, 1.0f, //Left  Bottom
                 1.0f,  1.0f,    1.0f, 0.0f, //Right Top

                 1.0f, -1.0f,    1.0f, 1.0f, //Right Bottom
                 1.0f,  1.0f,    1.0f, 0.0f, //Right Top  
                -1.0f, -1.0f,    0.0f, 1.0f  //Left  Bottom
            };

            constexpr static GLsizei s_VERTEX_COUNT = 6;

            //Shut up intellisense
            #ifdef __INTELLISENSE__
                static constexpr char s_VERTEX_SHADER_CODE[]   = {};
                static constexpr char s_FRAGMENT_SHADER_CODE[] = {};
            #else 
                static constexpr char s_VERTEX_SHADER_CODE[]   = { 
                    #embed "shaders/shader_FlatQuadImage.vert" suffix(, '\0') 
                };

                static constexpr char s_FRAGMENT_SHADER_CODE[] = { 
                    #embed "shaders/shader_FlatQuadImage.frag" suffix(, '\0') 
                };
            #endif

//-------------- Member variables
            Shader                    m_shader_program;
            VAO                       m_vao;
            VBO                       m_vbo;
            std::unique_ptr<Texture>  m_texture_ptr;

        public:

            explicit FlatQuadImage_RenderPipeline(const char* texture_path, const std::array<GLfloat, 24>& custom_quad = s_DEFAULT_QUAD) noexcept;

//----------------- Move is allowed
            explicit FlatQuadImage_RenderPipeline(FlatQuadImage_RenderPipeline&&)   noexcept = default;
            FlatQuadImage_RenderPipeline& operator=(FlatQuadImage_RenderPipeline&&) noexcept = default;

//----------------- No copying allowed
            FlatQuadImage_RenderPipeline(const FlatQuadImage_RenderPipeline&)            = delete;
            FlatQuadImage_RenderPipeline& operator=(const FlatQuadImage_RenderPipeline&) = delete;

//----------------- Methods
            void SetTexture(const char* texture_path) noexcept;
            void SetQuad(const std::array<GLfloat, 24>& custom_quad) noexcept;

            void Render() noexcept;
    };
}