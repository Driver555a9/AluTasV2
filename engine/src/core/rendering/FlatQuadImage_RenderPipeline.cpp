#include "core/rendering/FlatQuadImage_RenderPipeline.h"

namespace CoreEngine
{
    FlatQuadImage_RenderPipeline::FlatQuadImage_RenderPipeline(const char* texture_path, const std::array<GLfloat, 24>& custom_quad) noexcept
        : m_shader_program(s_VERTEX_SHADER_CODE, s_FRAGMENT_SHADER_CODE, Shader::ProvidedPointers::ARE_SOURCE_CODE), m_vbo(custom_quad)
    {
        m_vao.LinkAttribute(m_vbo, 0, 2, GL_FLOAT, 4 * sizeof(GLfloat), (void*) 0);
        m_vao.LinkAttribute(m_vbo, 1, 2, GL_FLOAT, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat))); 

        SetTexture(texture_path);
    }

    void FlatQuadImage_RenderPipeline::SetTexture(const char* texture_path) noexcept
    {
        m_texture_ptr = std::make_unique<Texture>(texture_path);
    }

    void FlatQuadImage_RenderPipeline::SetQuad(const std::array<GLfloat, 24>& custom_quad) noexcept
    {
        m_vbo.SetNewData(custom_quad);
    }

    void FlatQuadImage_RenderPipeline::Render() noexcept
    {
        if (!m_texture_ptr) [[unlikely]] 
        {   
            return;
        }
    //--------- For 2D no depth
        glDisable(GL_DEPTH_TEST);

    //-------- Ensure no issues through culling
        glDisable(GL_CULL_FACE);

    //-------- Draw Call
        m_vao.Bind(); // VAO implicitly also binds VBO

        m_shader_program.Activate();
        glProgramUniformHandleui64ARB(m_shader_program.GetID(), glGetUniformLocation(m_shader_program.GetID(), "tex_bindless_handle"), m_texture_ptr->GetBindlessHandle());

        glDrawArrays(GL_TRIANGLES, 0, s_VERTEX_COUNT);

        m_shader_program.Deactivate();

        m_vao.Unbind();
    }
}