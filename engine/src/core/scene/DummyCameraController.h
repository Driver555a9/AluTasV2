#pragma once

#include "core/scene/CameraController.h"

namespace CoreEngine
{
    class DummyCameraController : public Basic_CameraController
    {   
    public:
        explicit DummyCameraController() noexcept : Basic_CameraController(Type::Dummy) {}
        virtual ~DummyCameraController() noexcept = default;

        virtual void Update(CameraReverseZ& camera, const InputState& input_state, Units::Second dt) noexcept override {}
    };
}