#pragma once

#include "core/scene/CameraController.h"

namespace CoreEngine
{
    class OrbitalCam_CameraController : public Basic_CameraController
    {
    public:
        explicit OrbitalCam_CameraController() noexcept;
        virtual ~OrbitalCam_CameraController() noexcept override = default;

        virtual void Update(CameraReverseZ& camera, const InputState& input_state, Units::Second delta_time) noexcept override;

        [[nodiscard]] float GetSensitivity() const noexcept;
        [[nodiscard]] float GetDistance()    const noexcept;
        [[nodiscard]] float GetZoomSpeed()   const noexcept;
        [[nodiscard]] glm::vec3 GetTarget()  const noexcept;

        void SetSensitivity(float sensitivity) noexcept;
        void SetDistance(float distance)       noexcept;
        void SetZoomSpeed(float zoom_speed)    noexcept;
        void SetTarget(glm::vec3 target)       noexcept;

    protected:
        float m_distance     = 5.0f;

        glm::vec3 m_target   = {0, 0, 0};
        float m_sensitivity  = 0.1f;
        float m_zoom_speed   = 1.0f;
    };
}