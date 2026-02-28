#pragma once

#include "core/scene/CameraController.h"

namespace CoreEngine
{
    class DummyCameraController : public Basic_CameraController
    {   
    public:
        explicit DummyCameraController() noexcept : Basic_CameraController(Type::Dummy) {}
        virtual ~DummyCameraController() noexcept = default;

        virtual void Update([[maybe_unused]] CameraReverseZ& camera, [[maybe_unused]] const InputState& input_state, [[maybe_unused]] Units::Second dt) noexcept override {}
    };
}