#include "tas/GameState.h"

#include "libmem/libmem.hpp"

#include "tas/MemoryUtility.h"
#include "tas/MemoryRW.h"
#include "tas/MemoryAddressFinder.h"

#include <thread>

namespace AsphaltTas
{
    GameState::GamePlatform GameState::GetCurrentPlatform() noexcept
    {
        return s_platform.load(std::memory_order::acquire);
    }

    void GameState::LaunchDetectGameServiceThread() noexcept
    {
        std::thread([]() -> void
        {
            while (true)
            {
                std::optional<libmem::Process> opt_process;

                if ( (opt_process = libmem::FindProcess(GetGameExeNameFromPlatform(GamePlatform::STEAM))) )
                {
                    s_platform.store(GamePlatform::STEAM, std::memory_order::release);
                }
                else if ( (opt_process = libmem::FindProcess(GetGameExeNameFromPlatform(GamePlatform::MS))) )
                {
                    s_platform.store(GamePlatform::MS, std::memory_order::release);
                }

                if (opt_process.has_value())
                {
                    s_game_hwnd.store(MemoryUtility::GetHWNDFromPID(opt_process->pid), std::memory_order::release);
      
                    MemoryUtility::ApplicationShutdownWatchdog(opt_process->pid, &OnInvalidateAllCaches);

                    std::unique_lock lock(s_game_closed_mutex);
                    s_game_closed_cv.wait(lock, []{ return !GetHasValidCurrentPlatform(); });
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }).detach();
    }

    bool GameState::GetIsGameInForeground() noexcept
    {
        if (! GetHasValidCurrentPlatform()) 
            return false;

        try 
        {
            libmem::Process process = MemoryUtility::GetProcessOrThrow();
            return MemoryUtility::ProcessIsInForeground(process.pid);
        } 
        catch (...) { return false; }
    }

    bool GameState::GetHasValidCurrentPlatform() noexcept
    {
        GamePlatform platform = GetCurrentPlatform();
        return platform == GamePlatform::STEAM || platform == GamePlatform::MS;
    }

    HWND GameState::GetGameHWNDOrNullptr() noexcept
    {
        return s_game_hwnd.load(std::memory_order::acquire);
    }

    void GameState::OnInvalidateAllCaches() noexcept
    {
        s_platform.store(GamePlatform::NONE, std::memory_order::release);
        s_game_hwnd.store(nullptr, std::memory_order::release);
        MemoryUtility::InvalidateCache();
        MemoryRW::InvalidateCache();
        MemoryAddressFinder::InvalidateCache();
        s_game_closed_cv.notify_all();
    }


}