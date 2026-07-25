#include "CameraState.h"
#include "BulletTypes.h"
#include "Communication.h"
#include "glm/gtc/type_ptr.hpp"
#include <bit>

namespace AsphaltTas
{
    CameraState::CameraState(const ComDllIn::WriteCameraState& state) noexcept
    {
        m_position           = std::bit_cast<glm::vec3>(state.m_camera_position_vec3);
        m_rotation           = std::bit_cast<glm::quat>(state.m_camera_rotation_quat);
        m_local_racer_offset = std::bit_cast<glm::vec3>(state.m_offset_relative_to_car);;
        m_fov_radians        = state.m_fov_radians;
        m_look_backwards     = state.m_look_backwards;
    }

    CameraState::CameraState(const ComDllOut::RecordedCameraState& state) noexcept
    {
        m_position           = std::bit_cast<glm::vec3>(state.m_camera_position_vec3);
        m_rotation           = std::bit_cast<glm::quat>(state.m_camera_rotation_quat);
        m_local_racer_offset = std::bit_cast<glm::vec3>(state.m_offset_relative_to_car);
        m_fov_radians        = state.m_fov_radians;
        m_aspect_ratio       = state.m_aspect_ratio;
        m_look_backwards     = state.m_look_backwards;
    }

    ComDllIn::WriteCameraState CameraState::ToWriteCameraState() noexcept
    {
        ComDllIn::WriteCameraState state_out;
        state_out.m_camera_position_vec3   = std::bit_cast<BulletTypes::UnalignedVector3>(m_position);
        state_out.m_camera_rotation_quat   = std::bit_cast<BulletTypes::UnalignedQuaternion>(m_rotation);
        state_out.m_offset_relative_to_car = std::bit_cast<BulletTypes::UnalignedVector3>(m_local_racer_offset);
        state_out.m_fov_radians            = m_fov_radians;
        state_out.m_look_backwards         = m_look_backwards;
        return state_out;
    }

    ComDllOut::RecordedCameraState CameraState::ToRecordedCameraState() noexcept
    {
        ComDllOut::RecordedCameraState state_out;
        state_out.m_camera_position_vec3   = std::bit_cast<BulletTypes::UnalignedVector3>(m_position);
        state_out.m_camera_rotation_quat   = std::bit_cast<BulletTypes::UnalignedQuaternion>(m_rotation);
        state_out.m_offset_relative_to_car = std::bit_cast<BulletTypes::UnalignedVector3>(m_local_racer_offset);
        state_out.m_fov_radians    = m_fov_radians;
        state_out.m_aspect_ratio   = m_aspect_ratio;
        state_out.m_look_backwards = m_look_backwards;
        return state_out;
    }

    glm::vec3 CameraState::GetPositionOpenGL_XYZ() const noexcept
    {
        glm::vec3 xzy = m_position;
        std::swap(xzy.y, xzy.z);
        xzy.z *= -1.0f;
        return xzy;
    }

    void CameraState::SetPositionOpenGL_XYZ(const glm::vec3& pos) noexcept
    {
        m_position = pos;
        m_position.z *= -1.0f;
        std::swap(m_position.y, m_position.z);
    }

    glm::quat CameraState::GetRotationOpenGL_WXYZ() const noexcept
    {
        glm::quat out = m_rotation;
        std::swap(out.y, out.z);
        out.z *= -1.0f;
        return out;
    }

    void CameraState::SetRotationOpenGL_WXYZ(const glm::quat& rot) noexcept
    {
        m_rotation = rot;
        m_rotation.z *= -1.0f;
        std::swap(m_rotation.y, m_rotation.z);
    }

    glm::vec3 CameraState::GetPositionGameloft_XZY() const noexcept
    {
        return m_position;
    }

    void CameraState::SetPositionGameloft_XZY(const glm::vec3& pos) noexcept
    {
        m_position = pos;
    }

    glm::quat CameraState::GetRotationGameloft_XZYW() const noexcept
    {
        return m_rotation;
    }

    void CameraState::SetRotationGameloft_XZYW(const glm::quat& rot) noexcept
    {
        m_rotation = rot;
    }

    float CameraState::GetFovRadians() const noexcept
    {
        return m_fov_radians;
    }

    void CameraState::SetFovRadians(float fov) noexcept
    {
        m_fov_radians = fov;
    }

    float CameraState::GetAspectRatio() const noexcept
    {
        return m_aspect_ratio;
    }

    std::string CameraState::ToString() const noexcept 
    {
        return "Position" + CoreEngine::CommonUtility::GlmVec3ToString(GetPositionOpenGL_XYZ()) 
                + "\nRotation: " + CoreEngine::CommonUtility::GlmQuatToString(GetRotationOpenGL_WXYZ());
    }

}