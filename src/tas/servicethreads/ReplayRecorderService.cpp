#include "tas/servicethreads/ReplayRecorderService.h"

#include "tas/memory/MemoryRW.h"
#include "tas/memory/MemoryUtility.h"

#include "core/utility/Timer.h"
#include "core/utility/Assert.h"

//std
#include <utility>
#include <thread>

namespace AsphaltTas::ReplayRecorderService
{

namespace 
{
    Replay            g_replay;
    std::mutex        g_replay_mutex;
    std::atomic<bool> g_thread_is_running = false;
}

    void LaunchRecordThread() noexcept
    {
        if (GetThreadIsRunning()) return;

        g_thread_is_running.store(true, std::memory_order::release);

        std::thread ( []() -> void 
        { 
            CoreEngine::Timer timer;
            while (GetThreadIsRunning())
            {
                {
                    std::scoped_lock lock (g_replay_mutex);
                    try 
                    {
                        g_replay.EmplaceBackFrame(MemoryRW::ReadRacerState(), timer.GetElapsed<CoreEngine::Units::MicroSecond>());
                    }
                    catch (MemoryUtility::MemoryManipFailedException& e) 
                    { 
                        ENGINE_DEBUG_PRINT(e.what()); 
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }).detach();
    }

    void StopRecordThread() noexcept
    {
        g_thread_is_running.store(false, std::memory_order::release);
    }

    size_t GetAmountRecordedFrames() noexcept
    {
        std::scoped_lock lock(g_replay_mutex);
        return g_replay.GetAmountFrames();
    }

    Replay GetReplayCopy() noexcept
    {
        std::scoped_lock lock(g_replay_mutex);
        return g_replay;
    }

    void ClearAllRecordedStates() noexcept
    {
        std::scoped_lock lock(g_replay_mutex);
        g_replay.ClearAllFrameData();
    }

    bool GetThreadIsRunning() noexcept
    {
        return g_thread_is_running.load(std::memory_order::acquire);
    }
}