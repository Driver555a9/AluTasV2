#pragma once

//own includes
#include "core/rendering/Texture.h"
#include "core/rendering/RenderBuffers.h"
#include "core/rendering/Shader.h"

#include "core/model/Model.h"
#include "core/model/Light.h"

#include "core/rendering/Material.h"

namespace CoreEngine
{
    class IndirectDraw3D_RenderPipeline
    {
    public:
        //Maintain valid std140 alignment
        struct CameraRenderData
        {
            glm::mat4 m_cam_matrix;
            glm::vec3 m_camera_position; GLfloat padding;
        };
        //////////////////////////////////////////////// 
        //---------  Constructor
        //////////////////////////////////////////////// 
        explicit IndirectDraw3D_RenderPipeline() noexcept;

        //////////////////////////////////////////////// 
        //---------  Public methods
        //////////////////////////////////////////////// 
        //Call this is models were added / deleted - Expensive and will rebuild entire data once
        void SetSceneData(const std::vector<const Basic_Model*>& model_vec, const std::vector<Light>& lights) noexcept;
        //Call if no models added / deleted, but positions may have changed. Will apply frustum culling
        void UpdateModelTransforms(const std::vector<const Basic_Model*>& model_vec, const glm::mat4& camera_matrix) noexcept;
        //Call if just new light sources were added
        void SetLightData(const std::vector<Light>& lights) noexcept;
        //Call if camera state changed
        void SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept;
        [[nodiscard]] CameraRenderData GetCameraRenderData() const noexcept { return m_camera_render_data; }
        void Render() noexcept;

        [[nodiscard]] Shader& GetShaderProgramReference() noexcept;
        void RestoreShaderToDefault() noexcept;

        struct RenderConfig
        {
            enum class FrontFace {CCW, CW};
            FrontFace m_front_face = FrontFace::CCW;
            bool m_cull_back_face = true;
            bool m_use_cull_face  = true;
            bool m_frustum_cull   = true;
        };
        RenderConfig& GetRenderConfigRef() noexcept;

        //////////////////////////////////////////////// 
        //---------  Copy / Move policy
        //////////////////////////////////////////////// 
        explicit IndirectDraw3D_RenderPipeline(IndirectDraw3D_RenderPipeline&&)        = default;
        IndirectDraw3D_RenderPipeline& operator=(IndirectDraw3D_RenderPipeline&&)      = default;
        IndirectDraw3D_RenderPipeline(const IndirectDraw3D_RenderPipeline&)            = delete;
        IndirectDraw3D_RenderPipeline& operator=(const IndirectDraw3D_RenderPipeline&) = delete;

    protected:
        //////////////////////////////////////////////// 
        //---------  Data structs matching Shader
        ////////////////////////////////////////////////
        //Maintain valid std140 alignment
        struct SSBO_SizesData
        {
            GLuint m_active_draw_indices_size = 0;
            GLuint m_mesh_transform_size      = 0;
            GLuint m_materials_size           = 0;
            GLuint m_light_size               = 0;
        };
        
        //////////////////////////////////////////////// 
        //---------  CPU Side Data
        //////////////////////////////////////////////// 
        CameraRenderData                             m_camera_render_data;
        std::vector<GLuint>                          m_active_draw_indices;
        std::vector<glm::mat4>                       m_mesh_transforms;
        std::vector<std::shared_ptr<MaterialPBR>>    m_material_ptrs;
        RenderConfig                                 m_render_config;
        SSBO_SizesData m_cpu_ssbo_size_copy;

        //////////////////////////////////////////////// 
        //--------- GPU Side Data
        //////////////////////////////////////////////// 
        Shader  m_shader_program;
        
        UBO     m_ssbo_sizes_ubo;
        UBO     m_camera_ubo;

        SSBO    m_active_draw_indices_ssbo;
        SSBO    m_mesh_transform_ssbo;
        SSBO    m_texture_indices_ssbo;
        SSBO    m_light_ssbo;
        
        VBO     m_vbo;
        EBO     m_ebo;
        VAO     m_vao;

        GLsizei        m_draw_command_count = 0;
        IndirectBuffer m_indirect_command_buffer;

        //////////////////////////////////////////////// 
        //--------- Shaders
        //////////////////////////////////////////////// 
        static constexpr const char* s_VERTEX_SHADER_CODE = 
        #include "shaders/shader_IndirectDraw3D.vert" 
        ;
        
        static constexpr const char* s_FRAGMENT_SHADER_CODE = 
        #include "shaders/shader_IndirectDraw3D.frag" 
        ;
    };
}