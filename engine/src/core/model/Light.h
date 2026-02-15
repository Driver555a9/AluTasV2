#pragma once

//GLM
#include "glm/glm.hpp"

//Glad
#include "glad/gl.h"

#include <string>

namespace CoreEngine
{

    struct Light
    {
        /*If these change, change it in shader as well!*/
        enum LIGHT_MODE : GLuint 
        {
            OFF_LIGHT    = 0,
            POINT_LIGHT  = 1,
            SPOT_LIGHT   = 2,
            DIRECT_LIGHT = 3,

            FIRST = OFF_LIGHT,
            LAST  = DIRECT_LIGHT
        };

        glm::vec3 m_position {0.0f};   GLfloat m_intensity  {1.0f};         //12 + 4 bytes = 16 Byte alignment
        glm::vec3 m_color    {1.0f};   GLuint  m_light_mode {POINT_LIGHT};  //12 + 4 bytes = 16 Byte alignment

        constexpr inline explicit Light(const glm::vec3 _pos, const glm::vec3 _col, const GLfloat _intensity, const GLuint _light_mode) noexcept 
        : m_position(_pos), m_intensity(_intensity), m_color(_col), m_light_mode(_light_mode) {}
        
        constexpr inline explicit Light() noexcept = default;

        [[nodiscard]] constexpr static inline std::string LightModeToString(GLuint mode) noexcept
        {
            switch (mode)
            {
                case LIGHT_MODE::DIRECT_LIGHT: return "DIRECT_LIGHT";
                case LIGHT_MODE::POINT_LIGHT : return "POINT_LIGHT";
                case LIGHT_MODE::SPOT_LIGHT  : return "SPOT_LIGHT";
                case LIGHT_MODE::OFF_LIGHT   : return "OFF_LIGHT";
                default: return "UNKOWN LIGHT TYPE";
            }
        }
    };

    static_assert(sizeof(Light) == 32, "Light must be 24 bytes");
}