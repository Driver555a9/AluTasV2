#pragma once

//own includes
#include "core/rendering/Texture.h"
#include "core/rendering/RenderBuffers.h"
#include "core/rendering/Shader.h"

namespace CoreEngine
{
    class DrawLines3D_RenderPipeline
    {
    public:
        struct LineVertex
        {
            glm::vec3 m_position;
            glm::vec3 m_color;
        };

        static_assert(sizeof(LineVertex) == 24, "LineVertex must be 24 bytes");
        
    //////////////////////////////////////////////// 
    //---------  Methods
    //////////////////////////////////////////////// 
        template <typename... Args>
        requires std::is_constructible_v<LineVertex, Args...>
        void EmplaceBackLine(Args&&... args)
        {
            m_line_vertices.emplace_back(std::forward<Args>(args)...);
        }

        void SetCameraData(const glm::mat4& matrix) noexcept;
        void SetCameraDataAndFrustumCull(const glm::mat4& view_projection) noexcept;
        void SetLineData(const std::vector<glm::vec3>& line_vertex_positions, const glm::vec3& line_color) noexcept;
        void SetLineData(std::vector<LineVertex> line_vertices) noexcept;

        void SetLineThicknessFactor(float thickness) noexcept;
        [[nodiscard]] float GetLineThicknessFactor() const noexcept; 

        void ClearAllLines() noexcept;
        [[nodiscard]] size_t GetAmountLineVertices() const noexcept;
        void Render() noexcept;

    //////////////////////////////////////////////// 
    //---------  Constructors
    ////////////////////////////////////////////////
        explicit DrawLines3D_RenderPipeline() noexcept;
        ~DrawLines3D_RenderPipeline() noexcept = default;

    protected:
        static constexpr const char* s_VERTEX_SHADER_CODE = 
        #include "shaders/shader_DrawLines3D.vert" 
        ;
        
        static constexpr const char* s_FRAGMENT_SHADER_CODE = 
        #include "shaders/shader_DrawLines3D.frag" 
        ;

        static constexpr const char* s_GEOMETRY_SHADER_CODE = 
        #include "shaders/shader_DrawLines3D.geom" 
        ;

        Shader  m_shader_program;

        VBO     m_vbo;
        VAO     m_vao;

        GLint m_uniform_cam_matrix  = -1;

        std::vector<LineVertex> m_culled_draw_vertices;
        std::vector<LineVertex> m_line_vertices;

        float m_line_thickness_factor  = 1.0f;

        //////////////////////////////////////////////// 
        //---------  
        //////////////////////////////////////////////// 
        DrawLines3D_RenderPipeline(const DrawLines3D_RenderPipeline&)            = delete;
        DrawLines3D_RenderPipeline& operator=(const DrawLines3D_RenderPipeline&) = delete;
    };
}