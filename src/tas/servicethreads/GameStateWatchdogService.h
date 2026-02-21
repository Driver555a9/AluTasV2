#pragma once

namespace AsphaltTas
{
    namespace GameStateWatchdogService
    {
        void LaunchThread() noexcept;
        void StopThread() noexcept;
        [[nodiscard]] bool GetThreadIsRunning() noexcept;
    }
}