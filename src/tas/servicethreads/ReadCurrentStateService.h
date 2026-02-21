#pragma once

#include "tas/common/RacerState.h"
#include "tas/common/CameraState.h"

#include <optional>

namespace AsphaltTas
{
    namespace ReadCurrentStateService
    {
        void LaunchThread() noexcept;
        void StopThread() noexcept;
        [[nodiscard]] bool GetThreadIsRunning() noexcept;

        [[nodiscard]] std::optional<RacerState> GetInterpolatedRacerState() noexcept;
        [[nodiscard]] std::optional<RacerState> GetCurrentRacerState() noexcept;
        [[nodiscard]] std::optional<CameraState> GetCurrentCameraState() noexcept;
    }
}