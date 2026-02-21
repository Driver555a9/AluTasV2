#pragma once

#include <cstdint>
#include <stdexcept>

namespace AsphaltTas
{
    namespace MemoryAddressFinder
    {
        //////////////////////////////////////////////////////////
        // These functions may throw MemoryUtility::MemoryManipFailedException

        //////////////////////////////////////////////////////////
        [[nodiscard]] uintptr_t FindRacerStateBaseAddress();

        [[nodiscard]] uintptr_t FindCameraStateAddresses();

        /// Requires driving in race to find address
        [[deprecated("Use FinalCameraStateAddresses()")]]
        [[nodiscard]] uintptr_t FindActionCameraBaseAddress();

        void InvalidateCache() noexcept;
    };
    
}