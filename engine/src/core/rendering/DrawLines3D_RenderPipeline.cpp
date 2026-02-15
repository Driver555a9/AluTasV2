#include "core/rendering/DrawLines3D_RenderPipeline.h"

#include "core/utility/Performance.h"

namespace CoreEngine
{
    DrawLines3D_RenderPipeline::DrawLines3D_RenderPipeline() noexcept
    : m_shader_program(s_VERTEX_SHADER_CODE, s_FRAGMENT_SHADER_CODE, Shader::ProvidedPointers::ARE_SOURCE_CODE) 
    {
        m_vao.LinkAttribute(m_vbo, 0, 3, GL_FLOAT, sizeof(LineVertex), (void*)0);
        m_vao.LinkAttribute(m_vbo, 1, 3, GL_FLOAT, sizeof(LineVertex), (void*)sizeof(glm::vec3));
        m_uniform_cam_matrix = glGetUniformLocation(m_shader_program.GetID(), "camera_matrix_uniform");
    }

    void DrawLines3D_RenderPipeline::SetCameraMatrix(const glm::mat4& matrix) noexcept
    {
        glProgramUniformMatrix4fv(m_shader_program.GetID(), m_uniform_cam_matrix, 1, GL_FALSE, &matrix[0][0]);
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

    void DrawLines3D_RenderPipeline::SetLineData(std::vector<LineVertex>&& line_vertices) noexcept
    {
        m_line_vertices = std::move(line_vertices);
    }

    void DrawLines3D_RenderPipeline::SetCameraMatrixAndFrustumCull(const glm::mat4& view_projection) noexcept
    {
        SetCameraMatrix(view_projection);
        size_t lines_culled_count {0};

        //Backwards iteration required
        for (size_t i = m_line_vertices.size(); i >= 2; i -= 2)
        {
            size_t idx = i - 2;
            const MathUtility::Line line{ m_line_vertices[idx].m_position, m_line_vertices[idx+1].m_position };
            if (! MathUtility::LineIsInFrustum(MathUtility::ExtractProjectionPlanesFromVP(view_projection), line))
            {
                m_line_vertices.erase(m_line_vertices.begin() + idx, m_line_vertices.begin() + idx + 2);
                lines_culled_count++;
            }
        }

        ENGINE_PERFORMANCE_LOG_OCCURENCE("Line Frustum Culled: ", lines_culled_count);
    }

    void DrawLines3D_RenderPipeline::ClearAllLines() noexcept
    {
        m_line_vertices.clear();
    }

    void DrawLines3D_RenderPipeline::Render() noexcept
    {
        m_vbo.SetNewData(m_line_vertices.data(), static_cast<GLuint>(m_line_vertices.size() * sizeof(LineVertex)));

        m_vao.Bind();
        m_shader_program.Activate();

        glDrawArrays(GL_LINES, 0, m_line_vertices.size());

        m_vao.Unbind();
        m_shader_program.Deactivate();
    }

}