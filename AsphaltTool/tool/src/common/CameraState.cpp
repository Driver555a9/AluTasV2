#include "CameraState.h"
#include "Communication.h"
#include "glm/gtc/type_ptr.hpp"

namespace AsphaltTas
{
    CameraState::CameraState(const ComDllIn::WriteCameraState& state) noexcept
    {
        std::memcpy(glm::value_ptr(m_position), state.m_camera_position_vec3.data(), sizeof(decltype(m_position)));
        std::memcpy(glm::value_ptr(m_rotation), state.m_camera_rotation_quat.data(), sizeof(decltype(m_rotation)));
        m_fov_radians = state.m_fov_radians;
    }

    CameraState::CameraState(const ComDllOut::RecordedCameraState& state) noexcept
    {
        std::memcpy(glm::value_ptr(m_position), state.m_camera_position_vec3.data(), sizeof(decltype(m_position)));
        std::memcpy(glm::value_ptr(m_rotation), state.m_camera_rotation_quat.data(), sizeof(decltype(m_rotation)));
        m_fov_radians = state.m_fov_radians;
        m_aspect_ratio = state.m_aspect_ratio;
    }

    ComDllIn::WriteCameraState CameraState::ToWriteCameraState() noexcept
    {
        ComDllIn::WriteCameraState state_out;

        std::memcpy(state_out.m_camera_position_vec3.data(), glm::value_ptr(m_position), sizeof(decltype(m_position)));
        std::memcpy(state_out.m_camera_rotation_quat.data(), glm::value_ptr(m_rotation), sizeof(decltype(m_rotation)));
        state_out.m_fov_radians = m_fov_radians;
        return state_out;
    }

    ComDllOut::RecordedCameraState CameraState::ToRecordedCameraState() noexcept
    {
        ComDllOut::RecordedCameraState state_out;

        std::memcpy(state_out.m_camera_position_vec3.data(), glm::value_ptr(m_position), sizeof(decltype(m_position)));
        std::memcpy(state_out.m_camera_rotation_quat.data(), glm::value_ptr(m_rotation), sizeof(decltype(m_rotation)));
        state_out.m_fov_radians = m_fov_radians;
        state_out.m_aspect_ratio = m_aspect_ratio;
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