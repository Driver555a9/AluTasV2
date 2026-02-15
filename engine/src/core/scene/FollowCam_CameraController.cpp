#include "core/scene/FollowCam_CameraController.h"

#include <algorithm>

namespace CoreEngine
{
    FollowCam_CameraController::FollowCam_CameraController() noexcept : Basic_CameraController(Basic_CameraController::Type::FollowCam) {}

    void FollowCam_CameraController::Update(CameraReverseZ& camera, const InputState& input_state, Units::Second dt) noexcept
    {
        ////////////////////////////////////////////////
        // -------- Mouse Zoom Input (wheel scroll)
        ////////////////////////////////////////////////
        if (input_state.m_mouse_wheel_scroll_delta != 0.0f)
        {
            m_distance -= input_state.m_mouse_wheel_scroll_delta * m_zoom_speed * (1.0f + m_distance * 0.005f);
            m_distance = std::max(m_distance, 0.1f);
        }
        
        ////////////////////////////////////////////////
        // -------- Follow target input
        ////////////////////////////////////////////////
        if (input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_RIGHT])
        {
            m_yaw_offset   -= input_state.m_mouse_move_delta.x * 0.1f;
            m_pitch_offset -= input_state.m_mouse_move_delta.y * 0.1f;
        }

        m_pitch_offset = std::clamp(m_pitch_offset, -80.0f, 20.0f);

        const float basePitch = -20.0f; 
        
        const glm::quat qYaw   = glm::angleAxis(glm::radians(m_yaw_offset),   glm::vec3(0, 1, 0));
        const glm::quat qPitch = glm::angleAxis(glm::radians(m_pitch_offset + basePitch), glm::vec3(1, 0, 0));
        
        const glm::quat finalRot = m_target_rot * qYaw * qPitch;

        camera.SetRotation(finalRot);

        const glm::vec3 camForward = camera.GetForwardDirection();
        const glm::vec3 camPos     = m_target_pos - (camForward * m_distance);

        camera.SetPosition(camPos);
    }

    float FollowCam_CameraController::GetDistance() const noexcept
    {
        return m_distance;
    }

    float FollowCam_CameraController::GetZoomSpeed() const noexcept
    {
        return m_zoom_speed;
    }

    std::pair<glm::vec3, glm::quat> FollowCam_CameraController::GetTarget() const noexcept
    {
        return  {m_target_pos, m_target_rot};
    }

    void FollowCam_CameraController::SetTarget(glm::vec3 position, glm::quat rotation) noexcept
    {
        m_target_pos = position;
        m_target_rot = rotation;
    }

    void FollowCam_CameraController::SetDistance(float distance) noexcept
    {
        m_distance = distance;
    }

    void FollowCam_CameraController::SetZoomSpeed(float zoom_speed) noexcept
    {
        m_zoom_speed = zoom_speed;
    }   


}