#pragma once

#ifdef _WIN32

#include "glm/glm.hpp"

struct HWND__;
using HWND   = HWND__*;

namespace AsphaltTas
{
    namespace MouseInputService 
    {  
        // Will lock mouse pos if enabled + game is focussed
        void LaunchThread() noexcept;
        
        void StopThread() noexcept;

        [[nodiscard]] glm::ivec2 GetMouseDeltaMovementAndReset() noexcept;

        [[nodiscard]] bool GetThreadIsRunning() noexcept;

        [[nodiscard]] bool GetAlwaysRecenterCursor() noexcept;
        void SetAlwaysRecenterCursor(bool enable) noexcept;
    }

}

#endif