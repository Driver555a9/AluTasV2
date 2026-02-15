#include "tas/ReplayRecorder.h"

#include "tas/MemoryRW.h"
#include "tas/MemoryUtility.h"

#include "core/utility/Timer.h"

//std
#include <utility>
#include <thread>

namespace AsphaltTas
{
    ReplayRecorder::~ReplayRecorder() noexcept
    {
        OnStopRecordThread();
    }

    void ReplayRecorder::ThreadLoop(CoreEngine::Units::MicroSecond interval) noexcept
    {
        m_stop_flag.store(false);
        CoreEngine::Timer timer;
        timer.Restart();

        while (! m_stop_flag.load())
        {
            {
                std::scoped_lock lock (m_replay_mutex);
                try 
                {
                    m_replay.EmplaceBackFrame(MemoryRW::ReadRacerState(), timer.GetElapsed<CoreEngine::Units::MicroSecond>());
                }
                catch (MemoryUtility::MemoryManipFailedException& e) 
                { 
                    ENGINE_DEBUG_PRINT(e.what()); 
                    OnStopRecordThread(); 
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(interval.Get()));
        }
    }

    void ReplayRecorder::OnLaunchRecordThread(CoreEngine::Units::MicroSecond interval) noexcept
    {
        if ( GetThreadIsRunning() ) return;
        std::thread t ( [=, this] () -> void { ThreadLoop(interval); } );
        t.detach();
    }

    void ReplayRecorder::OnStopRecordThread() noexcept
    {
        m_stop_flag.store(true, std::memory_order::relaxed);
    }

    size_t ReplayRecorder::GetAmountRecordedFrames() noexcept
    {
        std::scoped_lock lock(m_replay_mutex);
        return m_replay.GetAmountFrames();
    }

    Replay ReplayRecorder::GetReplayCopy() noexcept
    {
        std::scoped_lock lock(m_replay_mutex);
        return m_replay;
    }

    void ReplayRecorder::ClearAllRecordedStates() noexcept
    {
        std::scoped_lock lock(m_replay_mutex);
        m_replay.ClearAllFrameData();
    }

    bool ReplayRecorder::GetThreadIsRunning() noexcept
    {
        return ! m_stop_flag.load();
    }
}