#pragma once

#include "core/utility/CommonUtility.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace AsphaltTas
{
    struct CameraState
    {
        glm::vec3 m_position {0};
        glm::quat m_rotation = glm::identity<glm::quat>();
        float     m_fov_radians   = 1.0f;
        float     m_aspect_ratio  = 1920.0f / 1080;

        [[nodiscard]] constexpr bool operator==(const CameraState& other) const noexcept
        {
            return m_position == other.m_position && m_rotation == other.m_rotation;
        }

        [[nodiscard]] constexpr std::string ToString() const noexcept 
        {
            return "Camera State:\nPosition" + CoreEngine::CommonUtility::GlmVec3ToString(m_position) 
                  + "\nRotation: " + CoreEngine::CommonUtility::GlmQuatToString(m_rotation);
        }
    };
}