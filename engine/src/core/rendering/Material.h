#pragma once

#include "Texture.h"

#include <memory>

namespace CoreEngine
{
    class MaterialPBR
    {
    public:
        glm::vec3 m_base_color_factor {1.0f};
        float m_metallic_factor       {0.0f};

        glm::vec3 m_emissive_factor   {0.0f};
        float m_roughness_factor      {1.0f};

        std::shared_ptr<Texture> m_base_texture               {nullptr};
        std::shared_ptr<Texture> m_metallic_roughness_texture {nullptr};
        std::shared_ptr<Texture> m_normal_texture             {nullptr};
        std::shared_ptr<Texture> m_occlusion_texture          {nullptr};
        std::shared_ptr<Texture> m_emissive_texture           {nullptr};

        struct BindlessHandle
        {
            GLuint m_low_32b  {0};
            GLuint m_high_32b {0};
        };

        struct GPU_std430_Aligned_Data
        {
            glm::vec3  m_base_color_factor {1.0}; 
            float m_metallic_factor  {0.0f};

            glm::vec3  m_emissive_factor {0.0};
            float m_roughness_factor {1.0};

            BindlessHandle m_base_color_tex_handle {};
            BindlessHandle m_metallic_roughness_tex_handle {};
            BindlessHandle m_normal_tex_handle {};
            BindlessHandle m_occlusion_tex_handle {};
            BindlessHandle m_emissive_tex_handle {};

            float _padding [2]; 
        };

        static_assert (sizeof(GPU_std430_Aligned_Data) == 80, "Expected sizeof(GPU_std430_Aligned_Data) to be 80 Bytes long");
        static_assert (sizeof(BindlessHandle) == 8, "Expected sizeof(BindlessHandle) to be 8 Bytes long");

        [[nodiscard]] constexpr inline GPU_std430_Aligned_Data GetGPUAlignedData() const noexcept
        {
            return GPU_std430_Aligned_Data 
            {
                .m_base_color_factor             = m_base_color_factor,
                .m_metallic_factor               = m_metallic_factor,
                .m_emissive_factor               = m_emissive_factor,
                .m_roughness_factor              = m_roughness_factor,
                .m_base_color_tex_handle         = BindlessHandleFromUint64(m_base_texture ? m_base_texture->GetBindlessHandle() : 0),
                .m_metallic_roughness_tex_handle = BindlessHandleFromUint64(m_metallic_roughness_texture ? m_metallic_roughness_texture->GetBindlessHandle() : 0),
                .m_normal_tex_handle             = BindlessHandleFromUint64(m_normal_texture ? m_normal_texture->GetBindlessHandle() : 0),
                .m_occlusion_tex_handle          = BindlessHandleFromUint64(m_occlusion_texture ? m_occlusion_texture->GetBindlessHandle() : 0),
                .m_emissive_tex_handle           = BindlessHandleFromUint64(m_emissive_texture ? m_emissive_texture->GetBindlessHandle() : 0)
            };
        }

    private:
        [[nodiscard]] constexpr inline static BindlessHandle BindlessHandleFromUint64(GLuint64 handle) noexcept
        {
            return BindlessHandle {.m_low_32b = GLuint(handle & 0xFFFFFFFFULL), .m_high_32b = GLuint((handle >> 32) & 0xFFFFFFFFULL)};
        }
    };
}