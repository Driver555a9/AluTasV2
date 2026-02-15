#pragma once

#include "core/scene/Camera.h"

#include "core/utility/Units.h"
#include "core/utility/InputState.h"

#include "core/event/InputEvents.h"

//std
#include <array>
#include <optional>

namespace CoreEngine
{
    class Basic_CameraController
    {
    public:
        enum class Type
        {
            NONE = 0,
            FreeCam,
            OrbitalCam,
            FollowCam,
            Dummy,
            Custom
        };

        virtual void Update(CameraReverseZ& camera, const InputState& input_state, Units::Second delta_time) noexcept = 0;
        [[nodiscard]] inline Type GetType() const noexcept { return m_type; }

        [[nodiscard]] inline float GetYaw()   const noexcept { return m_camera_yaw; }
        [[nodiscard]] inline float GetPitch() const noexcept { return m_camera_pitch; }
        
        void SetYaw(float yaw)     noexcept { m_camera_yaw = yaw; }
        void SetPitch(float pitch) noexcept { m_camera_pitch = pitch; }

        virtual ~Basic_CameraController() noexcept = default;

    protected:    
        explicit Basic_CameraController(Type type) noexcept : m_type(type) {}

        Type m_type;
        
        float m_camera_yaw   = 0.0f;
        float m_camera_pitch = 0.0f;
    };
}