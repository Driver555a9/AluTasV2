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

    std::atomic<bool> g_thread_is_running = false;
    std::mutex g_racer_state_mutex;
    std::mutex g_camera_state_mutex;
    std::optional<TimestampedRacerState> g_previous_racer_state = std::nullopt;
    std::optional<TimestampedRacerState> g_latest_racer_state   = std::nullopt;
    std::optional<CameraState> g_latest_camera_state            = std::nullopt;
}
    void LaunchThread() noexcept
    {
        if (GetThreadIsRunning()) return;

        g_thread_is_running.store(true);
        std::thread([]()
        {
            CoreEngine::Timer racer_sync_60_pf_timer;
            while (GetThreadIsRunning())
            {
                //if (racer_sync_60_pf_timer.GetElapsed<CoreEngine::Units::MilliSecond>() > CoreEngine::Units::MilliSecond(16))
                {
                    racer_sync_60_pf_timer.Restart();
                    std::scoped_lock lock(g_racer_state_mutex);
                    try 
                    {
                        const RacerState new_state = MemoryRW::ReadRacerState();

                        const bool changed = !g_latest_racer_state || !g_latest_racer_state->m_state.Equals(new_state);

                        if (changed)
                        {
                            g_previous_racer_state  = g_latest_racer_state;
                            g_latest_racer_state    = { new_state, CoreEngine::Timer::GetTimeSinceEpoch<CoreEngine::Units::Second>() };
                        }
    
                    } catch (...)
                    {
                        g_previous_racer_state = std::nullopt;
                        g_latest_racer_state = std::nullopt;
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

                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } 
            std::scoped_lock lock (g_racer_state_mutex, g_camera_state_mutex);
            g_latest_racer_state  = std::nullopt;
            g_latest_camera_state = std::nullopt;
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

    std::optional<RacerState> GetInterpolatedRacerState() noexcept
    {
        std::scoped_lock lock(g_racer_state_mutex);
        if (! g_latest_racer_state.has_value())
            return std::nullopt;

        const auto& state = g_latest_racer_state.value().m_state;

        constexpr float PHYSICS_STEP = 1.0f / 60.0f;
        constexpr float HALF_TICK    = PHYSICS_STEP * 0.5f;

        const glm::vec3 interpolated_pos = state.GetExtractedPosition() - state.GetVelocity() * HALF_TICK;
        
        RacerState copy = state;
        copy.SetPosition(interpolated_pos);
        return copy;
    }

    std::optional<RacerState> GetCurrentRacerState() noexcept
    {
        std::scoped_lock lock(g_racer_state_mutex);
        if (! g_latest_racer_state.has_value())
            return std::nullopt;

        return g_latest_racer_state->m_state;
    }

    std::optional<CameraState> GetCurrentCameraState() noexcept
    {
        std::scoped_lock lock(g_camera_state_mutex);
        return g_latest_camera_state;
    }
}