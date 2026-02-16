#include "tas/GameState.h"

#include "libmem/libmem.hpp"

#include "tas/MemoryUtility.h"
#include "tas/MemoryRW.h"
#include "tas/MemoryAddressFinder.h"
#include "tas/MouseInputService.h"

#include <thread>

namespace AsphaltTas
{
    GameState::GamePlatform GameState::GetCurrentPlatform() noexcept
    {
        return s_platform.load(std::memory_order::acquire);
    }

    void GameState::LaunchDetectGameServiceThread() noexcept
    {
        s_detect_game_thread_is_running.store(true, std::memory_order::release);
        std::thread([]() -> void
        {
            while (DetectGameServiceThreadIsRunning())
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
                    HWND hwnd = MemoryUtility::GetHWNDFromPID(opt_process->pid);
                    s_game_hwnd.store(hwnd, std::memory_order::release);
                    //MouseInputService::InitializeRawInputCapture();
                        
                    MemoryUtility::ApplicationShutdownWatchdog(opt_process->pid, &OnInvalidateAllCaches);

                    std::unique_lock lock(s_game_closed_mutex);
                    s_game_closed_cv.wait(lock, []{ return !GetHasValidCurrentPlatform(); });

                    //MouseInputService::ShutdownRawInputCapture();
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            s_detect_game_thread_is_running.store(false, std::memory_order::release);
        }).detach();
    }

    bool GameState::DetectGameServiceThreadIsRunning() noexcept
    {
        return s_detect_game_thread_is_running.load(std::memory_order::relaxed);
    }

    bool GameState::GetIsGameInForeground() noexcept
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
        MemoryUtility::InvalidateCache();
        MemoryRW::InvalidateCache();
        MemoryAddressFinder::InvalidateCache();
        s_platform.store(GamePlatform::NONE, std::memory_order::release);
        s_game_hwnd.store(nullptr, std::memory_order::release);
        s_game_closed_cv.notify_all();
    }


}