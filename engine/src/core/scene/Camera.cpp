#include "core/scene/Camera.h"

//own
#include "core/utility/Assert.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/MathUtility.h"
#include <optional>

namespace CoreEngine
{
    CameraReverseZ::CameraReverseZ(const glm::vec3& position, const float aspect_ratio, const float fov_deg, const float near_plane, glm::quat rot) noexcept
        :   m_position(position),
            m_rotation(rot),
            m_aspect_ratio(aspect_ratio),
            m_near_plane(near_plane),
            m_fov_rad(glm::radians(fov_deg))
    {}

    /// @return Returns reverse Z View-Projection matrix
    glm::mat4 CameraReverseZ::CalculateCameraMatrix(const std::optional<glm::vec3>& target_to_look_at) const noexcept 
    {
        m_cached_matrix = CalculateProjectionMatrix() * CalculateViewMatrix(target_to_look_at);
        m_has_cached_matrix = true;
        return m_cached_matrix;
    }

    glm::mat4 CameraReverseZ::CalculateViewMatrix(const std::optional<glm::vec3>& target_to_look_at) const noexcept  
    {
        if(target_to_look_at.has_value())
            return glm::lookAt(m_position, target_to_look_at.value(), GetAbsoluteUp());
        else 
        {
            glm::mat4 rotation    = glm::mat4_cast(glm::inverse(m_rotation));
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
            return rotation * translation;
        }
    }

    /// @return Reverse Z Projection matrix
    glm::mat4 CameraReverseZ::CalculateProjectionMatrix() const noexcept 
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

    void CameraReverseZ::SetPosition(const glm::vec3& position)    noexcept { m_position     = position;                 InvalidateCache(); }
    void CameraReverseZ::Move(const glm::vec3& movement)		   noexcept { m_position     += movement;                InvalidateCache(); }
    void CameraReverseZ::SetRotation(const glm::quat& rotation)    noexcept { m_rotation     = glm::normalize(rotation); InvalidateCache(); }
    void CameraReverseZ::SetAspectRatio(float ratio)               noexcept { m_aspect_ratio = ratio;                    InvalidateCache(); }
    void CameraReverseZ::SetNearPlane(float nearPlane)             noexcept { m_near_plane 	 = nearPlane;                InvalidateCache(); }
    void CameraReverseZ::SetFovRad(float fov)                      noexcept { m_fov_rad    	 = fov;                      InvalidateCache(); }
    void CameraReverseZ::SetFovDeg(float fov) 		               noexcept { m_fov_rad      = glm::radians(fov);        InvalidateCache(); }

    glm::vec3 CameraReverseZ::GetForwardDirection()  const noexcept { return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f); }
    glm::vec3 CameraReverseZ::GetRightDirection()    const noexcept { return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);  }

    glm::mat4 CameraReverseZ::GetLastCachedCameraMatrix() const noexcept { ENGINE_ASSERT(HasCachedCameraMatrix() && "No cached matrix avaiable"); return m_cached_matrix; }
	bool CameraReverseZ::HasCachedCameraMatrix() const noexcept { return m_has_cached_matrix; }
    glm::vec3 CameraReverseZ::GetPosition() const noexcept { return m_position; 	}
    glm::quat CameraReverseZ::GetRotation() const noexcept { return m_rotation; 	}
    float CameraReverseZ::GetNearPlane()    const noexcept { return m_near_plane;   }
    float CameraReverseZ::GetFovRad()       const noexcept { return m_fov_rad; 	    }
    float CameraReverseZ::GetFovDeg()       const noexcept { return glm::degrees(m_fov_rad); }	
    float CameraReverseZ::GetAspectRatio()  const noexcept { return m_aspect_ratio; }
    std::string CameraReverseZ::ToString()  const noexcept { return "Pos: " + CommonUtility::GlmVec3ToString(m_position) + "\nRot: " + CommonUtility::GlmQuatToString(m_rotation); }

    void CameraReverseZ::InvalidateCache() noexcept { m_has_cached_matrix = false; }
}