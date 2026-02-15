#pragma once

#include "core/scene/CameraController.h"

namespace CoreEngine
{
    class FollowCam_CameraController : public Basic_CameraController
    {
    public:
        explicit FollowCam_CameraController() noexcept;
        virtual ~FollowCam_CameraController() noexcept = default;
    
        virtual void Update(CameraReverseZ& camera, const InputState& input_state, Units::Second dt) noexcept override;

        [[nodiscard]] float GetDistance()    const noexcept;
        [[nodiscard]] float GetZoomSpeed()   const noexcept;
        [[nodiscard]] std::pair<glm::vec3, glm::quat> GetTarget()  const noexcept;

        void SetTarget(glm::vec3 position, glm::quat rotation) noexcept;
        void SetDistance(float distance)       noexcept;
        void SetZoomSpeed(float zoom_speed)    noexcept;

    private:
        glm::vec3 m_target_pos;
        glm::quat m_target_rot;

        float m_yaw_offset   = 0.0f;
        float m_pitch_offset = -10.0f;
        float m_distance     = 5.0f;
        float m_zoom_speed   = 1.0f;
    };
}