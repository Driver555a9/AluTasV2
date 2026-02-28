#include "tas/servicethreads/ReadCurrentStateService.h"

#include "tas/memory/MemoryRW.h"

#include "core/utility/Assert.h"

#include "core/utility/Timer.h"

#include <atomic>
#include <thread>
#include <mutex>

namespace AsphaltTas::ReadCurrentStateService
{
namespace
{
    struct TimestampedRacerState 
    {
        RacerState m_state;
        CoreEngine::Units::Second m_timestamp;
    };

    std::mutex g_racer_state_mutex;
    std::optional<TimestampedRacerState> g_previous_racer_state = std::nullopt;
    std::optional<TimestampedRacerState> g_latest_racer_state   = std::nullopt;

    std::mutex g_camera_state_mutex;
    std::optional<CameraState> g_latest_camera_state = std::nullopt;

    std::mutex g_race_progress_mutex;
    std::optional<RaceProgressState> g_latest_race_progress_state = std::nullopt;

    std::atomic<bool> g_thread_is_running = false;
}
    void LaunchThread() noexcept
    {
        if (GetThreadIsRunning()) return;

        g_thread_is_running.store(true);
        std::thread([]()
        {
            ENGINE_INFO_LOG("Read Current State Thread launched.");
            while (GetThreadIsRunning())
            {
                {
                    std::scoped_lock lock(g_racer_state_mutex);
                    try 
                    {
                        const RacerState new_state = MemoryRW::ReadRacerState();

                        const bool changed = !g_latest_racer_state || !g_latest_racer_state->m_state.Equals(new_state);

                        if (changed)
                        {
                            const CoreEngine::Units::Second now_ts = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::Second>();
                            g_previous_racer_state = g_latest_racer_state;
                            g_latest_racer_state   = { new_state, now_ts };
                        }
    
                    } 
                    catch (...)
                    {
                        g_previous_racer_state = std::nullopt;
                        g_latest_racer_state   = std::nullopt;
                    }
                }

                {
                    std::scoped_lock lock(g_camera_state_mutex);
                    try 
                    {
                        g_latest_camera_state = MemoryRW::ReadCameraState();
                    } catch (...)
                    {
                        g_latest_camera_state = std::nullopt;
                    }
                }

                {
                    std::scoped_lock lock(g_race_progress_mutex);
                    try 
                    {
                        g_latest_race_progress_state = RaceProgressState();
                        g_latest_race_progress_state->m_lap_time = MemoryRW::ReadCurrentRaceTime();
                        g_latest_race_progress_state->m_race_progress_percentage = MemoryRW::ReadCurrentRaceProgress();
                    } 
                    catch (...)
                    {
                        g_latest_race_progress_state = std::nullopt;
                    }
                    
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } 
            std::scoped_lock lock (g_racer_state_mutex, g_camera_state_mutex, g_race_progress_mutex);
            g_latest_racer_state  = std::nullopt;
            g_latest_camera_state = std::nullopt;
            g_latest_race_progress_state = std::nullopt;
            ENGINE_INFO_LOG("Read Current State Thread stopped.");
        }).detach();
    }

    void StopThread() noexcept
    {
        g_thread_is_running.store(false, std::memory_order::release);
    }

    bool GetThreadIsRunning() noexcept
    {
        return g_thread_is_running.load(std::memory_order::acquire);
    }

    std::optional<RacerState> GetCurrentRacerState() noexcept
    {
        std::scoped_lock lock(g_racer_state_mutex);
        if (! g_latest_racer_state.has_value())
            return std::nullopt;

        RacerState copy = g_latest_racer_state->m_state;
        copy.SetPosition(copy.GetExtractedPosition() - (copy.GetVelocity() * (1/120.0f)));
        return copy;
    }

    std::optional<CameraState> GetCurrentCameraState() noexcept
    {
        std::scoped_lock lock(g_camera_state_mutex);
        return g_latest_camera_state;
    }

    std::optional<RaceProgressState> GetCurrentRaceProgressState() noexcept
    {
        std::scoped_lock lock(g_race_progress_mutex);
        return g_latest_race_progress_state;
    }
}