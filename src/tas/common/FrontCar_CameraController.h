#pragma once

#include "core/scene/CameraController.h"
#include "core/utility/Timer.h"

#include "tas/common/RacerState.h"


namespace AsphaltTas
{
    class FrontCar_CameraControler : public CoreEngine::Basic_CameraController
    {
    public:
        explicit FrontCar_CameraControler() noexcept;
        virtual ~FrontCar_CameraControler() noexcept override = default;

        virtual void Update(CoreEngine::CameraReverseZ& camera, const CoreEngine::InputState& input_state, CoreEngine::Units::Second delta_time) noexcept override;

        void SetRacerState(RacerState state) noexcept;

        [[nodiscard]] float GetOffsetForward() noexcept;
        void SetOffsetForward(float offset) noexcept;

        [[nodiscard]] float GetOffsetUp() noexcept;
        void SetOffsetUp(float offset) noexcept;

        [[nodiscard]] float GetOffsetRight() noexcept;
        void SetOffsetRight(float offset) noexcept;

        [[nodiscard]] bool GetLookBackwards() noexcept;
        void SetLookBackwards(bool look_backwards) noexcept;
        
    private:
        RacerState m_racer_state {};
        glm::vec3  m_offset = glm::vec3(0.0f, 1.0f, 3.0f);
        bool       m_look_backwards  = false;
    };
}