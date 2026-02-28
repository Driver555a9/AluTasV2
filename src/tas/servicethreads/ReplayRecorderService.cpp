#include "tas/servicethreads/ReplayRecorderService.h"

#include "tas/memory/MemoryRW.h"
#include "tas/memory/MemoryUtility.h"
#include "tas/servicethreads/ReadCurrentStateService.h"
#include "tas/globalstate/MemoryAddressState.h"

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

    ////////////////////////
    // Only used to detect new race
    ////////////////////////
    uintptr_t g_progress_address = 0x0;
    uintptr_t g_lap_time_address = 0x0;
}

    void LaunchRecordThread() noexcept
    {
        if (GetThreadIsRunning()) return;

        g_replay.ClearAllFrameData();
        g_thread_is_running.store(true, std::memory_order::release);

        std::thread ( []() -> void 
        { 
            ENGINE_INFO_LOG("Replay Recorder Thread launched.");
            //CoreEngine::Timer timer;
            while (GetThreadIsRunning())
            {
                {
                std::scoped_lock lock(g_replay_mutex);
                try
                {
                    // Detect restart or new race by decreased percentage, time or insanely small percentage 
                    if ((g_lap_time_address != RaceProgressStateAddresses::GetLapTimeAddress()) || (g_progress_address != RaceProgressStateAddresses::GetRaceProgressAddress()))
                    {
                        g_lap_time_address = RaceProgressStateAddresses::GetLapTimeAddress();
                        g_progress_address = RaceProgressStateAddresses::GetRaceProgressAddress();
                        g_replay.ClearAllFrameData();
                    }
                    
                    const std::optional<RacerState> race_state = ReadCurrentStateService::GetCurrentRacerState();
                    const std::optional<RaceProgressState> progress_state = ReadCurrentStateService::GetCurrentRaceProgressState();

                    if (race_state.has_value() && progress_state.has_value())
                    {

                        const std::optional<Replay::Frame> last_frame = g_replay.GetLastFrame();

                        // No previous frame means always update as long as valid
                        if (!last_frame.has_value())
                        {
                            if (progress_state->m_race_progress_percentage > 0.0f && progress_state->m_race_progress_percentage <= 100.0f)
                            {
                                g_replay.EmplaceBackFrame(race_state.value(), progress_state.value());
                            }
                        }
                        else
                        {
                            const float last_percentage  = last_frame->m_race_progress_state.m_race_progress_percentage;
                            const float new_percentage   = progress_state->m_race_progress_percentage;
                            const CoreEngine::Units::MicroSecond last_time = last_frame->m_race_progress_state.m_lap_time;
                            const CoreEngine::Units::MicroSecond new_time  = progress_state->m_lap_time;

                            //////////////////////////////////////////////////////
                            /// Finish Detection:
                            /// > new progress >= 100%
                            /// > last progress < 100%
                            //////////////////////////////////////////////////////
                            const bool is_finish_frame = (new_percentage >= 100.0f && last_percentage < 100.0f);

                            //////////////////////////////////////////////////////
                            /// Normal update:
                            /// > progress between 0 and 100%
                            /// > lap time increased
                            /// > last frame the race was not already finished
                            //////////////////////////////////////////////////////
                            const bool is_normal_update = (new_percentage > 0.0f && new_percentage <= 100.0f) && (new_time > last_time) && (last_percentage < 100.0f);

                            if (is_finish_frame || is_normal_update)
                            {
                                g_replay.EmplaceBackFrame(race_state.value(), progress_state.value());
                            }
                        }
                    }
                }
                catch (...) {}
            }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            ENGINE_INFO_LOG("Replay Recorder Thread stopped.");
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

    std::optional<Replay::Frame> GetLastFrame() noexcept
    {
        std::scoped_lock lock(g_replay_mutex);
        return g_replay.GetLastFrame();
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