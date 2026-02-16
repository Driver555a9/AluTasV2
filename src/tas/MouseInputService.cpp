#include "tas/MouseInputService.h"

#ifdef _WIN32

#include <windows.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <utility>

#include "tas/GameState.h"

#include "core/utility/Timer.h"

namespace AsphaltTas::MouseInputService
{

namespace 
{
    std::atomic<bool> g_thread_is_running    {false};
    glm::ivec2        g_delta_mouse_movement {0, 0};
    std::mutex        g_delta_movement_mutex;
    std::atomic<bool> g_always_recenter_cursor {true};
}

    void LaunchMouseCaptureThread() noexcept
    {
        g_thread_is_running.store(true, std::memory_order::release);
        std::thread([]() 
        {
            auto GetPos = []() -> glm::ivec2 {
                POINT p;
                GetCursorPos(&p);
                return { p.x, p.y };
            };

            glm::ivec2 last_pos = GetPos();
            CoreEngine::Timer reset_cursor_timer;

            constexpr CoreEngine::Units::MicroSecond interval_reset {500};

            while (GetThreadIsRunning())
            {
                while (! GameState::GetIsGameInForeground() && GetThreadIsRunning())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                {
                    std::scoped_lock lock(g_delta_movement_mutex);
                    const glm::ivec2 pos_now = GetPos();
                    g_delta_mouse_movement += (pos_now - last_pos);
                    last_pos = pos_now;
                }
                if ( (reset_cursor_timer.GetElapsed<CoreEngine::Units::MicroSecond>() > interval_reset)  && g_always_recenter_cursor.load(std::memory_order::relaxed))
                {
                    SetCursorPos(100, 100);
                    last_pos = {100, 100};
                    reset_cursor_timer.Restart();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }).detach();
        
        std::scoped_lock lock(g_delta_movement_mutex);
        g_delta_mouse_movement = {0, 0};
    }
        
    void StopMouseCaptureThread() noexcept
    {
        g_thread_is_running.store(false, std::memory_order::release);
    }

    glm::ivec2 GetMouseDeltaMovementAndReset() noexcept
    {
        std::scoped_lock lock(g_delta_movement_mutex);
        return std::exchange(g_delta_mouse_movement, {0, 0});
    }

    bool GetThreadIsRunning() noexcept
    {
        return g_thread_is_running.load(std::memory_order::acquire);
    }

    bool GetAlwaysRecenterCursor() noexcept
    {
        return g_always_recenter_cursor.load(std::memory_order::relaxed);
    }

    void SetAlwaysRecenterCursor(bool enable) noexcept
    {
        g_always_recenter_cursor.store(enable, std::memory_order::relaxed);
    }
}

#endif