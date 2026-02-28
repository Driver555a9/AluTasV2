#pragma once

#include "tas/common/Replay.h"

#include "core/utility/Units.h"

#include <mutex>
#include <atomic>

namespace AsphaltTas
{
    namespace ReplayRecorderService
    {
        void LaunchRecordThread() noexcept;
        void StopRecordThread() noexcept;

        [[nodiscard]] size_t GetAmountRecordedFrames() noexcept;
        [[nodiscard]] Replay GetReplayCopy() noexcept;
        [[nodiscard]] std::optional<Replay::Frame> GetLastFrame() noexcept;

        void ClearAllRecordedStates() noexcept;

        [[nodiscard]] bool GetThreadIsRunning() noexcept;
    };
}