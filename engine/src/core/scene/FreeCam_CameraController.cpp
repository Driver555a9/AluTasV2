#include "core/scene/FreeCam_CameraController.h"

#include <algorithm>

namespace CoreEngine
{
    FreeCam_CameraController::FreeCam_CameraController() noexcept : Basic_CameraController(Basic_CameraController::Type::FreeCam) {}

    void FreeCam_CameraController::Update(CameraReverseZ& camera, const InputState& input_state, Units::Second delta_time) noexcept
    {
    //////////////////////////////////////////////// 
    //---------  Key movement input
    //////////////////////////////////////////////// 
        glm::vec3 movement(0.0f);

        if (input_state.m_key_is_pressed[GLFW_KEY_W])            movement.z -= 1.0f;
        if (input_state.m_key_is_pressed[GLFW_KEY_S])            movement.z += 1.0f;
        if (input_state.m_key_is_pressed[GLFW_KEY_A])            movement.x -= 1.0f;
        if (input_state.m_key_is_pressed[GLFW_KEY_D])            movement.x += 1.0f;
        if (input_state.m_key_is_pressed[GLFW_KEY_SPACE])        movement.y += 1.0f;
        if (input_state.m_key_is_pressed[GLFW_KEY_LEFT_CONTROL]) movement.y -= 1.0f;

        if (glm::length(movement) > 0.0f)
        {
            const glm::vec3 world_movement = camera.GetRightDirection()   *  movement.x  + 
                                             camera.GetAbsoluteUp()       *  movement.y  +
                                             camera.GetForwardDirection() * -movement.z;

            camera.Move(glm::normalize(world_movement) * m_move_speed * static_cast<float>(Units::Convert<Units::Second>(delta_time).Get()));
        }

    //////////////////////////////////////////////// 
    //---------  Mouse turn input
    ////////////////////////////////////////////////
        if (! input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_RIGHT]) //Only turn camera while right mouse is pressed
            return;

        m_camera_yaw   -= input_state.m_mouse_move_delta.x * m_sensitivity;
        m_camera_pitch -= input_state.m_mouse_move_delta.y * m_sensitivity;
        m_camera_pitch = std::clamp(m_camera_pitch, -89.0f, 89.0f);

        const glm::quat qYaw   = glm::angleAxis(glm::radians(m_camera_yaw), glm::vec3(0, 1, 0));
        const glm::vec3 right  = qYaw * glm::vec3(1, 0, 0); 
        const glm::quat qPitch = glm::angleAxis(glm::radians(m_camera_pitch), right);

        const glm::quat rotation = qPitch * qYaw;

        camera.SetRotation(rotation);

    //////////////////////////////////////////////// 
    //--------- Scrollwheel fov change
    ////////////////////////////////////////////////
        float fov_now = camera.GetFovDeg();
        fov_now -= input_state.m_mouse_wheel_scroll_delta * 2.0f;
        fov_now = std::clamp(fov_now, 20.0f, 120.0f);
        camera.SetFovDeg(fov_now);
    }

    
    float FreeCam_CameraController::GetMoveSpeed() const noexcept
    {
        return m_move_speed;
    }

    float FreeCam_CameraController::GetSensitivity() const noexcept
    {
        return m_sensitivity;
    }
    
    void FreeCam_CameraController::SetMoveSpeed(float speed) noexcept
    {
        m_move_speed = speed;
    }   

    void FreeCam_CameraController::SetSensitivity(float sensitivity) noexcept
    {
        m_sensitivity = sensitivity;
    }
}