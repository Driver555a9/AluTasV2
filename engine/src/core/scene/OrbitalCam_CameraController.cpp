#include "core/scene/OrbitalCam_CameraController.h"

#include <algorithm>

namespace CoreEngine
{
    OrbitalCam_CameraController::OrbitalCam_CameraController() noexcept : Basic_CameraController(Basic_CameraController::Type::OrbitalCam) {}

    void OrbitalCam_CameraController::Update(CameraReverseZ& camera, const InputState& input_state, Units::Second delta_time) noexcept
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
        // -------- Mouse Rotation Input
        ////////////////////////////////////////////////
        if (input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_RIGHT])
        {
            m_camera_yaw   -= input_state.m_mouse_move_delta.x * m_sensitivity;
            m_camera_pitch -= input_state.m_mouse_move_delta.y * m_sensitivity;
            m_camera_pitch = std::clamp(m_camera_pitch, -89.0f, 89.0f);
        }

        ////////////////////////////////////////////////
        // -------- Build Quaternion Rotation
        ////////////////////////////////////////////////
        const glm::quat qYaw   = glm::angleAxis(glm::radians(m_camera_yaw),   glm::vec3(0, 1, 0));
        const glm::vec3 right  = qYaw * glm::vec3(1, 0, 0);
        const glm::quat qPitch = glm::angleAxis(glm::radians(m_camera_pitch), right);

        const glm::quat rotation = qPitch * qYaw;

        camera.SetRotation(rotation);

        ////////////////////////////////////////////////
        // -------- Update Orbital Position
        ////////////////////////////////////////////////
        const glm::vec3 forward = camera.GetForwardDirection();
        const glm::vec3 new_pos = m_target - forward * m_distance;

        camera.SetPosition(new_pos);
    }

    float OrbitalCam_CameraController::GetSensitivity() const noexcept
    {
        return m_sensitivity;
    }

    float OrbitalCam_CameraController::GetDistance() const noexcept
    {
        return m_distance;
    }

    float OrbitalCam_CameraController::GetZoomSpeed() const noexcept
    {
        return m_zoom_speed;
    }

    glm::vec3 OrbitalCam_CameraController::GetTarget() const noexcept
    {
        return m_target;
    }

    void OrbitalCam_CameraController::SetSensitivity(float sensitivity) noexcept
    {
        m_sensitivity = sensitivity;
    }

    void OrbitalCam_CameraController::SetDistance(float distance) noexcept
    {
        m_distance = distance;
    }

    void OrbitalCam_CameraController::SetZoomSpeed(float zoom_speed) noexcept
    {
        m_zoom_speed = zoom_speed;
    }

    void OrbitalCam_CameraController::SetTarget(glm::vec3 target) noexcept
    {
        m_target = target;
    }
}