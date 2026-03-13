#include "tas/rendering/DrawGhost_RenderPipeline.h"

#include "core/model/BoxModel.h"
#include "core/utility/Assert.h"

namespace AsphaltTas
{
    DrawGhost_RenderPipeline::DrawGhost_RenderPipeline() noexcept
    {
        m_draw_line_pipeline.SetLineThicknessFactor(10.0f);
    }

    void DrawGhost_RenderPipeline::SetGhostData(const CoreEngine::Basic_Model* model) noexcept
    {
        CoreEngine::Light light {model->GetPosition(), glm::vec3(0.9f, 0.9f, 1.0f), 500.0f, CoreEngine::Light::LIGHT_MODE::DIRECT_LIGHT};
        m_indirect_pipeline.SetSceneData({ model }, {light});
    }

    void DrawGhost_RenderPipeline::SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept
    {
        m_indirect_pipeline.SetCameraData(cam_matrix, cam_pos);
        m_draw_line_pipeline.SetCameraData(cam_matrix);
    }

    void DrawGhost_RenderPipeline::SetRacingLineData(std::vector<CoreEngine::DrawLines3D_RenderPipeline::LineVertex> line_vertices) noexcept
    {
        m_draw_line_pipeline.SetLineData(std::move(line_vertices));
    }

    glm::vec4 DrawGhost_RenderPipeline::GetGhostColor() const noexcept
    {
        return m_ghost_color;
    }

    void DrawGhost_RenderPipeline::SetGhostColor(const glm::vec4& color) noexcept
    {
        //May be called from a different context; therefore assign collor when context is assured to be right.
        m_ghost_color   = color;
        m_color_changed = true;
    }

    bool DrawGhost_RenderPipeline::GetIsUsingCustomGhostColorShader() noexcept
    {
        return m_use_custom_ghost_color_shader;
    }
    
    void DrawGhost_RenderPipeline::SetUseCustomGhostColorShader(bool on) noexcept
    {
        m_use_custom_color_shader_changed = true;
        m_use_custom_ghost_color_shader = on;
    }

    //////////////////////////////////////
    // Only here is it really guaranteed to be our context
    //////////////////////////////////////
    void DrawGhost_RenderPipeline::Render() noexcept
    {
        if (m_use_custom_color_shader_changed)
        {
            if (m_use_custom_ghost_color_shader)
            {
                m_indirect_pipeline.GetShaderProgramReference() = CoreEngine::Shader(s_GHOST_VERTEX_SHADER_CODE, s_GHOST_FRAGMENT_SHADER_CODE, nullptr, 
                                                                  CoreEngine::Shader::ProvidedPointers::ARE_SOURCE_CODE);
                m_color_changed = true;
            }
            else  
            {
                m_indirect_pipeline.RestoreShaderToDefault(); 
            }
            m_use_custom_color_shader_changed = false;
        }

        if (m_color_changed && m_use_custom_ghost_color_shader)
        {
            const GLuint id = m_indirect_pipeline.GetShaderProgramReference().GetID();
            const GLint location = glGetUniformLocation(id, "ghost_color_uniform");
            glProgramUniform4f(id, location, m_ghost_color.r, m_ghost_color.g, m_ghost_color.b, m_ghost_color.w);
            m_color_changed = false;
        }

        m_indirect_pipeline.Render();
        m_draw_line_pipeline.Render();
    }

}