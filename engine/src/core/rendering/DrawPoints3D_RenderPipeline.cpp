#include "core/rendering/DrawPoints3D_RenderPipeline.h"

namespace CoreEngine
{
    DrawPoints3D_RenderPipeline::DrawPoints3D_RenderPipeline() noexcept
    : m_shader_program(s_VERTEX_SHADER_CODE, s_FRAGMENT_SHADER_CODE, Shader::ProvidedPointers::ARE_SOURCE_CODE) 
    {
        m_vao.LinkAttribute(m_vbo, 0, 3, GL_FLOAT, sizeof(PointVertex), (void*)0);
        m_vao.LinkAttribute(m_vbo, 1, 3, GL_FLOAT, sizeof(PointVertex), (void*)sizeof(glm::vec3));
        m_uniform_cam_matrix  = glGetUniformLocation(m_shader_program.GetID(), "camera_matrix_uniform");
        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    void DrawPoints3D_RenderPipeline::SetPoints(const std::vector<glm::vec3>& points, const glm::vec3& color) noexcept
    { 
        std::vector<PointVertex> point_vertices;
        point_vertices.reserve(points.size());
        for (const glm::vec3& pos : points)
        {
            point_vertices.emplace_back(pos, color);
        }
        SetPoints(std::move(point_vertices));
    }

    void DrawPoints3D_RenderPipeline::SetPoints(std::vector<PointVertex>&& points) noexcept
    {
        m_point_vertices = std::move(points);
    }

    void DrawPoints3D_RenderPipeline::SetCameraMatrix(const glm::mat4& matrix) noexcept
    {
        glProgramUniformMatrix4fv(m_shader_program.GetID(), m_uniform_cam_matrix, 1, GL_FALSE, &matrix[0][0]);
    }

    void DrawPoints3D_RenderPipeline::ClearAllPoints() noexcept
    {
        m_point_vertices.clear();
    }

    void DrawPoints3D_RenderPipeline::Render() noexcept
    {
        m_vbo.SetNewData(m_point_vertices.data(), static_cast<GLuint>(m_point_vertices.size() * sizeof(PointVertex)));

        m_vao.Bind();
        m_shader_program.Activate();

        glDrawArrays(GL_POINTS, 0, m_point_vertices.size());

        m_vao.Unbind();
        m_shader_program.Deactivate();
    }
}