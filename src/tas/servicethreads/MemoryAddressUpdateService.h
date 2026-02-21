#pragma once

namespace AsphaltTas
{
    namespace MemoryAddressUpdateService
    {
        void LaunchThread() noexcept;
        void StopThread() noexcept;
        [[nodiscard]] bool GetThreadIsRunning() noexcept;
    }
}