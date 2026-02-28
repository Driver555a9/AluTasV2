#pragma once

#include <cstdint>
#include <stdexcept>

#include "core/utility/Units.h"

namespace AsphaltTas
{
    namespace MemoryAddressFinder
    {
        //////////////////////////////////////////////////////////
        // These functions may throw MemoryUtility::MemoryManipFailedException

        //////////////////////////////////////////////////////////
        [[nodiscard]] uintptr_t FindRacerStateBaseAddress(CoreEngine::Units::MilliSecond max_wait_time = CoreEngine::Units::MilliSecond(500));

        [[nodiscard]] uintptr_t FindCameraStateAddresses(CoreEngine::Units::MilliSecond max_wait_time = CoreEngine::Units::MilliSecond(500));

        /// Requires driving in race to find address
        [[deprecated("Use FinalCameraStateAddresses()")]]
        [[nodiscard]] uintptr_t FindActionCameraBaseAddress(CoreEngine::Units::MilliSecond max_wait_time = CoreEngine::Units::MilliSecond(500));

        [[nodiscard]] uintptr_t FindLapTimeAddress(CoreEngine::Units::MilliSecond max_wait_time = CoreEngine::Units::MilliSecond(500));
        [[nodiscard]] uintptr_t FindRaceProgressAddress(CoreEngine::Units::MilliSecond max_wait_time = CoreEngine::Units::MilliSecond(500));
        //[[nodiscard]] uintptr_t FindCheckpointAddress();

        void InvalidateCache() noexcept;
    };
    
}