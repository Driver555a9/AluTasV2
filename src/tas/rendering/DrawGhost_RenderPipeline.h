#pragma once

#include "core/rendering/IndirectDraw3D_RenderPipeline.h"
#include "core/rendering/DrawLines3D_RenderPipeline.h"

namespace AsphaltTas
{
    class DrawGhost_RenderPipeline
    {
    public:
        explicit DrawGhost_RenderPipeline() noexcept;

        void SetGhostData(const CoreEngine::Basic_Model* model) noexcept;
        void SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept;
        void SetRacingLineData(std::vector<CoreEngine::DrawLines3D_RenderPipeline::LineVertex> line_vertices) noexcept;

        [[nodiscard]] glm::vec4 GetGhostColor() const noexcept;
        void SetGhostColor(const glm::vec4& color) noexcept;

        bool GetIsUsingCustomGhostColorShader() noexcept;
        void SetUseCustomGhostColorShader(bool on) noexcept;

        void Render() noexcept;

    private:
        CoreEngine::IndirectDraw3D_RenderPipeline m_indirect_pipeline;
        CoreEngine::DrawLines3D_RenderPipeline m_draw_line_pipeline;

        glm::vec4 m_ghost_color {1, 0, 0, 1};
        bool m_color_changed = true;

        bool m_use_custom_ghost_color_shader = true;
        bool m_use_custom_color_shader_changed = true; //Set once on first time
        //////////////////////////////////////////////// 
        //--------- Shaders
        //////////////////////////////////////////////// 
        #ifdef __INTELLISENSE__
            static constexpr char s_GHOST_VERTEX_SHADER_CODE[]   = {};
            static constexpr char s_GHOST_FRAGMENT_SHADER_CODE[] = {};
        #else 
            static constexpr char s_GHOST_VERTEX_SHADER_CODE[]   = { 
                #embed "shader_Ghost.vert" suffix(, '\0') 
            };
            
            static constexpr char s_GHOST_FRAGMENT_SHADER_CODE[] = { 
                #embed "shader_Ghost.frag" suffix(, '\0') 
            };
        #endif
    };
}