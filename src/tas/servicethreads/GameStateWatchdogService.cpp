#include "tas/servicethreads/GameStateWatchdogService.h"

#include "tas/globalstate/GameState.h"
#include "tas/memory/MemoryUtility.h"

#include "libmem/libmem.hpp"

#include <atomic>
#include <thread>

namespace AsphaltTas::GameStateWatchdogService
{

namespace
{
    std::atomic<bool> g_thread_is_running;
    std::condition_variable g_wait_for_game_close_cv;
    std::mutex g_game_closed_mutex;

    void GameCloseCallback() noexcept
    {
        GameState::OnInvalidateAllCaches();
        g_wait_for_game_close_cv.notify_all();
    }
}

    void LaunchThread() noexcept
    {
        if (GetThreadIsRunning()) return;

        g_thread_is_running.store(true, std::memory_order::release);

        std::thread([]()
        {
            while (GetThreadIsRunning())
            {
                std::optional<libmem::Process> opt_process;

                if ( (opt_process = libmem::FindProcess(GameState::GetGameExeNameFromPlatform(GameState::GamePlatform::STEAM))) )
                {
                    GameState::SetCurrentPlatform(GameState::GamePlatform::STEAM);
                }
                else if ( (opt_process = libmem::FindProcess(GameState::GetGameExeNameFromPlatform(GameState::GamePlatform::MS))) )
                {
                    GameState::SetCurrentPlatform(GameState::GamePlatform::MS);
                }

                if (opt_process.has_value())
                {
                    GameState::SetHWND(MemoryUtility::GetHWNDFromPID(opt_process->pid));
                        
                    MemoryUtility::LaunchApplicationShutdownWatchdogThread(opt_process->pid, &GameCloseCallback);

                    std::unique_lock lock(g_game_closed_mutex);
                    // GameCloseCallback() invalidates platform such that it GameState::GetHasValidCurrentPlatform() would return false
                    g_wait_for_game_close_cv.wait(lock, []{ return !GameState::GetHasValidCurrentPlatform() || !GetThreadIsRunning(); });
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }).detach();
    }

    void StopThread() noexcept
    {
        g_thread_is_running.store(false, std::memory_order::release);
        g_wait_for_game_close_cv.notify_all();
    }

    bool GetThreadIsRunning() noexcept
    {
        return g_thread_is_running.load(std::memory_order::acquire);
    }

}