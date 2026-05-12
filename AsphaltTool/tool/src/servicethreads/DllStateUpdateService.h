#pragma once

namespace AsphaltTas
{
    namespace DllStateUpdateService
    {
        void LaunchThread() noexcept;
        void StopThread() noexcept;
        [[nodiscard]] bool GetThreadIsRunning() noexcept;
    }
}