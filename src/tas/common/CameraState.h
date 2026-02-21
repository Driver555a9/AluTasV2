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

        [[nodiscard]] constexpr std::string ToString() const noexcept 
        {
            return "Position" + CoreEngine::CommonUtility::GlmVec3ToString(m_position) 
                  + "\nRotation: " + CoreEngine::CommonUtility::GlmQuatToString(m_rotation);
        }
    };
}