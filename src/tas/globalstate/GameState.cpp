#include "tas/globalstate/GameState.h"

#include "libmem/libmem.hpp"

#include "tas/memory/MemoryUtility.h"
#include "tas/memory/MemoryRW.h"
#include "tas/memory/MemoryAddressFinder.h"

#include <thread>

namespace AsphaltTas::GameState
{

namespace
{
    std::atomic<GamePlatform> g_platform  = GamePlatform::NONE;
    std::atomic<HWND>         g_game_hwnd = nullptr;
}

    void SetCurrentPlatform(GamePlatform platform) noexcept
    {
        g_platform.store(platform, std::memory_order::release);
    }

    GamePlatform GetCurrentPlatform() noexcept
    {
        return g_platform.load(std::memory_order::acquire);
    }

    bool GetIsGameInForeground() noexcept
    {
        if (! GetHasValidCurrentPlatform()) 
            return false;

        try 
        {
            libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();
            return MemoryUtility::ProcessIsInForeground(process.pid);
        } 
        catch (...) { return false; }
    }

    bool GetHasValidCurrentPlatform() noexcept
    {
        GamePlatform platform = GetCurrentPlatform();
        return platform == GamePlatform::STEAM || platform == GamePlatform::MS;
    }

    void SetHWND(HWND hwnd) noexcept
    {
        g_game_hwnd.store(hwnd, std::memory_order::release);
    }

    HWND GetGameHWNDOrNullptr() noexcept
    {
        return g_game_hwnd.load(std::memory_order::acquire);
    }

    void OnInvalidateAllCaches() noexcept
    {
        MemoryUtility::InvalidateCache();
        MemoryRW::InvalidateCache();
        MemoryAddressFinder::InvalidateCache();
        g_platform.store(GamePlatform::NONE, std::memory_order::release);
        g_game_hwnd.store(nullptr, std::memory_order::release);
    }


}