#pragma once

#include "core/utility/Assert.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

struct HWND__;
using HWND   = HWND__*;

namespace AsphaltTas
{
    namespace GameState
    {
        enum class GamePlatform
        {
            NONE, MS, STEAM
        };

        void SetCurrentPlatform(GamePlatform platform) noexcept;

        [[nodiscard]] GamePlatform GetCurrentPlatform() noexcept;

        [[nodiscard]] bool GetIsGameInForeground() noexcept;

        [[nodiscard]] bool GetHasValidCurrentPlatform() noexcept;

        void SetHWND(HWND hwnd) noexcept;
        [[nodiscard]] HWND GetGameHWNDOrNullptr() noexcept;

        constexpr const char* const ASPHALT_EXE_NAME_MS    = "Asphalt9_gdk_x64_rtl.exe";
        constexpr const char* const ASPHALT_EXE_NAME_STEAM = "Asphalt9_Steam_x64_rtl.exe";

        [[nodiscard]] constexpr const char* GetGameExeNameFromPlatform(GamePlatform platform) noexcept
        {
            if (platform == GamePlatform::MS)
                return ASPHALT_EXE_NAME_MS;
            if (platform == GamePlatform::STEAM)
                return ASPHALT_EXE_NAME_STEAM;
            ENGINE_ASSERT(false && "Expected a valid platform to convert into EXE name.");
        }

        void OnInvalidateAllCaches() noexcept;
    };
}