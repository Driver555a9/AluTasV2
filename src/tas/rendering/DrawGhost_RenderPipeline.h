#pragma once

#include "core/rendering/IndirectDraw3D_RenderPipeline.h"

namespace AsphaltTas
{
    class DrawGhost_RenderPipeline : private CoreEngine::IndirectDraw3D_RenderPipeline
    {
    public:
        explicit DrawGhost_RenderPipeline() noexcept;

        void SetGhostData(const CoreEngine::Basic_Model* model) noexcept;
        void SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept;

        [[nodiscard]] glm::vec4 GetColor() const noexcept;
        void SetColor(const glm::vec4& color) noexcept;

        bool GetIsUsingCustomColorShader() noexcept;
        void SetUseCustomColorShader(bool on) noexcept;

        void Render() noexcept;


    private:
        glm::vec4 m_color {1, 0, 0, 1};
        bool m_color_changed = true;

        bool m_use_custom_color_shader = true;
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