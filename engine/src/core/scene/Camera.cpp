#include "core/scene/Camera.h"

//own
#include "core/utility/CommonUtility.h"
#include "core/utility/MathUtility.h"

namespace CoreEngine
{
    CameraReverseZ::CameraReverseZ(const glm::vec3& position, const float aspect_ratio, const float fov_deg, const float near_plane, glm::quat rot)
        :   m_position(position),
            m_rotation(rot),
            m_aspect_ratio(aspect_ratio),
            m_near_plane(near_plane),
            m_fov_rad(glm::radians(fov_deg))
    {}

    /// @return Returns reverse Z View-Projection matrix
    glm::mat4 CameraReverseZ::CalculateCameraMatrix(const std::optional<glm::vec3>& target_to_look_at) const
    {
        return CalculateProjectionMatrix() * CalculateViewMatrix(target_to_look_at);
    }

    glm::mat4 CameraReverseZ::CalculateViewMatrix(const std::optional<glm::vec3>& target_to_look_at) const 
    {
        if(target_to_look_at.has_value())
            return glm::lookAt(m_position, target_to_look_at.value(), GetAbsoluteUp());
        else 
            return glm::lookAt(m_position, m_position + GetForwardDirection(), GetAbsoluteUp());
    }

    /// @return Reverse Z Projection matrix
    glm::mat4 CameraReverseZ::CalculateProjectionMatrix() const 
    {
        const float f = 1.0f / tan(m_fov_rad * 0.5f);

        glm::mat4 proj(0.0f);

        proj[0][0] = f / m_aspect_ratio;
        proj[1][1] = f;

        proj[2][2] = 0.0f; 
        proj[2][3] = -1.0f;
        proj[3][2] = m_near_plane;

        return proj;
    }

	std::array<glm::vec4, 5> CameraReverseZ::GetViewProjPlanes(const std::optional<glm::vec3>& target_to_look_at) const noexcept
    {
        return MathUtility::ExtractProjectionPlanesFromVP(CalculateCameraMatrix(target_to_look_at));
    }

    void CameraReverseZ::SetPosition(const glm::vec3& position)    noexcept { m_position     = position;  }
    void CameraReverseZ::Move(const glm::vec3& movement)		   noexcept { m_position     += movement; }
    void CameraReverseZ::SetRotation(const glm::quat& rotation)    noexcept { m_rotation     = rotation;  }
    void CameraReverseZ::SetAspectRatio(float ratio)               noexcept { m_aspect_ratio = ratio; 	  }
    void CameraReverseZ::SetNearPlane(float nearPlane)             noexcept { m_near_plane 	 = nearPlane; }
    void CameraReverseZ::SetFovRad(float fov)                      noexcept { m_fov_rad    	 = fov;       }
    void CameraReverseZ::SetFovDeg(float fov) 		               noexcept { m_fov_rad = glm::radians(fov); }

    glm::vec3 CameraReverseZ::GetForwardDirection()  const noexcept { return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f); }
    glm::vec3 CameraReverseZ::GetRightDirection()    const noexcept { return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);  }

    glm::vec3 CameraReverseZ::GetPosition() const noexcept { return m_position; 	}
    glm::quat CameraReverseZ::GetRotation() const noexcept { return m_rotation; 	}
    float CameraReverseZ::GetNearPlane()    const noexcept { return m_near_plane;   }
    float CameraReverseZ::GetFovRad()       const noexcept { return m_fov_rad; 	    }
    float CameraReverseZ::GetFovDeg()       const noexcept { return glm::degrees(m_fov_rad); }	
    float CameraReverseZ::GetAspectRatio()  const noexcept { return m_aspect_ratio; }
    std::string CameraReverseZ::ToString()  const noexcept { return "Pos: " + CommonUtility::GlmVec3ToString(m_position) + "\nRot: " + CommonUtility::GlmQuatToString(m_rotation); }
}