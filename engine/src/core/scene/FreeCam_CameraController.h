#pragma once

#include "core/scene/CameraController.h"

namespace CoreEngine
{
    class FreeCam_CameraController : public Basic_CameraController 
    {
    public:
        explicit FreeCam_CameraController() noexcept;
        virtual ~FreeCam_CameraController() noexcept override = default;

        virtual void Update(CameraReverseZ& camera, const InputState& input_state, Units::Second delta_time) noexcept override;

        [[nodiscard]] float GetMoveSpeed() const noexcept;
        [[nodiscard]] float GetSensitivity() const noexcept;
        
        void SetMoveSpeed(const float speed) noexcept;
        void SetSensitivity(const float sensitivity) noexcept;

    protected:
        float m_move_speed     {15.0f};
        float m_sensitivity    {0.1f};
    };
}