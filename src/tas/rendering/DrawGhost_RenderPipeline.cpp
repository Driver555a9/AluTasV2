#include "tas/rendering/DrawGhost_RenderPipeline.h"

#include "core/model/BoxModel.h"
#include "core/utility/Assert.h"

namespace AsphaltTas
{
    DrawGhost_RenderPipeline::DrawGhost_RenderPipeline() noexcept : CoreEngine::IndirectDraw3D_RenderPipeline()
    {

    }

    void DrawGhost_RenderPipeline::SetGhostData(const CoreEngine::Basic_Model* model) noexcept
    {
        CoreEngine::Light light {model->GetPosition(), glm::vec3(0.9f, 0.9f, 1.0f), 500.0f, CoreEngine::Light::LIGHT_MODE::DIRECT_LIGHT};
        IndirectDraw3D_RenderPipeline::SetSceneData({ model }, {light});
    }

    void DrawGhost_RenderPipeline::SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept
    {
        IndirectDraw3D_RenderPipeline::SetCameraData(cam_matrix, cam_pos);
    }

    glm::vec4 DrawGhost_RenderPipeline::GetColor() const noexcept
    {
        return m_color;
    }

    void DrawGhost_RenderPipeline::SetColor(const glm::vec4& color) noexcept
    {
        //May be called from a different context; therefore assign collor when context is assured to be right.
        m_color = color;
        m_color_changed = true;
    }

    
    bool DrawGhost_RenderPipeline::GetIsUsingCustomColorShader() noexcept
    {
        return m_use_custom_color_shader;
    }
    
    void DrawGhost_RenderPipeline::SetUseCustomColorShader(bool on) noexcept
    {
        m_use_custom_color_shader_changed = true;
        m_use_custom_color_shader = on;
    }

    //////////////////////////////////////
    // Only here is it really guaranteed to be our context
    //////////////////////////////////////
    void DrawGhost_RenderPipeline::Render() noexcept
    {
        if (m_use_custom_color_shader_changed)
        {
            if (m_use_custom_color_shader)
            {
                m_shader_program = CoreEngine::Shader(s_GHOST_VERTEX_SHADER_CODE, s_GHOST_FRAGMENT_SHADER_CODE, CoreEngine::Shader::ProvidedPointers::ARE_SOURCE_CODE);
                m_color_changed = true;
            }
            else 
            {
                using ID3D = IndirectDraw3D_RenderPipeline;
                m_shader_program = CoreEngine::Shader(ID3D::s_VERTEX_SHADER_CODE, ID3D::s_FRAGMENT_SHADER_CODE, CoreEngine::Shader::ProvidedPointers::ARE_SOURCE_CODE);
            }
            m_use_custom_color_shader_changed = false;
        }

        if (m_color_changed && m_use_custom_color_shader)
        {
            const GLint location = glGetUniformLocation(m_shader_program.GetID(), "ghost_color_uniform");
            glProgramUniform4f(m_shader_program.GetID(), location, m_color.r, m_color.g, m_color.b, m_color.w);
            m_color_changed = false;
        }

        IndirectDraw3D_RenderPipeline::Render();
    }

}