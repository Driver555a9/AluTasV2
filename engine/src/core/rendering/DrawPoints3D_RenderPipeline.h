#pragma once

#include "core/rendering/Texture.h"
#include "core/rendering/RenderBuffers.h"
#include "core/rendering/Shader.h"

namespace CoreEngine
{
    class DrawPoints3D_RenderPipeline
    {
    public:
        struct PointVertex
        {
            glm::vec3 m_position;
            glm::vec3 m_color;
        };

        static_assert(sizeof(PointVertex) == 24, "PointVertex must be 24 bytes");

        explicit DrawPoints3D_RenderPipeline() noexcept;

        template <typename... Args>
        requires std::is_constructible_v<PointVertex, Args...>
        void EmplaceBackPoint(Args&&... args)
        {
            m_point_vertices.emplace_back(std::forward<Args>(args)...);
        }
        
        void SetPoints(const std::vector<glm::vec3>& points, const glm::vec3& color) noexcept;
        void SetPoints(std::vector<PointVertex>&& points) noexcept;
        void SetCameraMatrix(const glm::mat4& cam_matrix) noexcept;
        void ClearAllPoints() noexcept;
        
        void Render() noexcept;

    protected:
        static constexpr const char* s_VERTEX_SHADER_CODE = 
        #include "shaders/shader_DrawPoints3D.vert" 
        ;
        
        static constexpr const char* s_FRAGMENT_SHADER_CODE = 
        #include "shaders/shader_DrawPoints3D.frag" 
        ;

        Shader  m_shader_program;

        VBO     m_vbo;
        VAO     m_vao;

        GLint m_uniform_cam_matrix  = -1;

        std::vector<PointVertex> m_point_vertices;
};
}