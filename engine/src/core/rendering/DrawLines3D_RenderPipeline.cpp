#include "core/rendering/DrawLines3D_RenderPipeline.h"

#include "core/utility/Performance.h"

namespace CoreEngine
{
    DrawLines3D_RenderPipeline::DrawLines3D_RenderPipeline() noexcept
    : m_shader_program(s_VERTEX_SHADER_CODE, s_FRAGMENT_SHADER_CODE, s_GEOMETRY_SHADER_CODE, Shader::ProvidedPointers::ARE_SOURCE_CODE) 
    {
        m_vao.LinkAttribute(m_vbo, 0, 3, GL_FLOAT, sizeof(LineVertex), (void*)0);
        m_vao.LinkAttribute(m_vbo, 1, 3, GL_FLOAT, sizeof(LineVertex), (void*)sizeof(glm::vec3));
        m_uniform_cam_matrix = glGetUniformLocation(m_shader_program.GetID(), "camera_matrix_uniform");
        SetLineThicknessFactor(m_line_thickness_factor);
    }

    void DrawLines3D_RenderPipeline::SetCameraData(const glm::mat4& matrix) noexcept
    {
        glProgramUniformMatrix4fv(m_shader_program.GetID(), m_uniform_cam_matrix, 1, GL_FALSE, &matrix[0][0]);
        m_culled_draw_vertices.clear();
    }

    void DrawLines3D_RenderPipeline::SetCameraDataAndFrustumCull(const glm::mat4& view_projection) noexcept
    {
        SetCameraData(view_projection);
        size_t lines_culled_count {0};
        m_culled_draw_vertices = m_line_vertices;
        //Backwards iteration required for deletion
        for (size_t i = m_culled_draw_vertices.size(); i >= 2; i -= 2)
        {
            size_t idx = i - 2;
            const MathUtility::Line line { m_culled_draw_vertices[idx].m_position, m_culled_draw_vertices[idx+1].m_position };
            if (! MathUtility::LineIsInFrustum(MathUtility::ExtractProjectionPlanesFromVP(view_projection), line))
            {
                m_culled_draw_vertices.erase(m_culled_draw_vertices.begin() + idx, m_culled_draw_vertices.begin() + idx + 2);
                lines_culled_count++;
            }
        }

        ENGINE_PERFORMANCE_LOG_OCCURENCE("Line Frustum Culled: ", lines_culled_count);
    }

    void DrawLines3D_RenderPipeline::SetLineData(const std::vector<glm::vec3>& line_vertex_positions, const glm::vec3& line_color) noexcept
    {
        std::vector<LineVertex> lines;
        lines.reserve(line_vertex_positions.size());
        for (const glm::vec3& pos : line_vertex_positions)
        {
            lines.emplace_back(pos, line_color);
        }
        SetLineData(std::move(lines));
    }

    void DrawLines3D_RenderPipeline::SetLineData(std::vector<LineVertex> line_vertices) noexcept
    {
        m_line_vertices = std::move(line_vertices);
        m_culled_draw_vertices.clear();
    }

    void DrawLines3D_RenderPipeline::SetLineThicknessFactor(float thickness) noexcept
    {
        m_line_thickness_factor = thickness;
        glProgramUniform1f(m_shader_program.GetID(), glGetUniformLocation(m_shader_program.GetID(), "line_thickness_factor"), m_line_thickness_factor * 0.01f); //Scale to NDC space
    }

    float DrawLines3D_RenderPipeline::GetLineThicknessFactor() const noexcept
    {
        return m_line_thickness_factor;
    }

    void DrawLines3D_RenderPipeline::ClearAllLines() noexcept
    {
        m_line_vertices.clear();
        m_culled_draw_vertices.clear();
    }

    void DrawLines3D_RenderPipeline::Render() noexcept
    {
        const std::vector<LineVertex>& draw_data = m_culled_draw_vertices.empty() ? m_line_vertices : m_culled_draw_vertices;

        m_vbo.SetNewData(draw_data.data(), static_cast<GLuint>(draw_data.size() * sizeof(LineVertex)));

        m_vao.Bind();
        m_shader_program.Activate();
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(draw_data.size()));
        m_vao.Unbind();
        m_shader_program.Deactivate();
    }

}