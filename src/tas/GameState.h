#pragma once

#include "core/utility/Assert.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

struct HWND__;
using HWND   = HWND__*;

namespace AsphaltTas
{
    class GameState
    {
    public:
        enum class GamePlatform
        {
            NONE, MS, STEAM
        };

        GameState() noexcept = delete;

        static void LaunchDetectGameServiceThread() noexcept;

        [[nodiscard]] static GamePlatform GetCurrentPlatform() noexcept;

        [[nodiscard]] static bool GetIsGameInForeground() noexcept;

        [[nodiscard]] static bool GetHasValidCurrentPlatform() noexcept;

        [[nodiscard]] static HWND GetGameHWNDOrNullptr() noexcept;

        static constexpr const char* const ASPHALT_EXE_NAME_MS    = "Asphalt9_gdk_x64_rtl.exe";
        static constexpr const char* const ASPHALT_EXE_NAME_STEAM = "Asphalt9_Steam_x64_rtl.exe";

        [[nodiscard]] constexpr static const char* GetGameExeNameFromPlatform(GamePlatform platform) noexcept
        {
            if (platform == GamePlatform::MS)
                return ASPHALT_EXE_NAME_MS;
            if (platform == GamePlatform::STEAM)
                return ASPHALT_EXE_NAME_STEAM;
            ENGINE_ASSERT(false && "Expected a valid platform to convert into EXE name.");
        }

        static void OnInvalidateAllCaches() noexcept;

    private:
        static inline std::atomic<GamePlatform> s_platform  = GamePlatform::NONE;
        static inline std::atomic<HWND>         s_game_hwnd = nullptr;

        static inline std::condition_variable s_game_closed_cv;
        static inline std::mutex s_game_closed_mutex;
    };
}