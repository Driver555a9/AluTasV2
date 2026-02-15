#pragma once

//std
#include <cstdint>
#include <chrono>

#include "core/utility/Units.h"
#include "core/utility/Assert.h"

namespace CoreEngine
{
    class Timer
    {   
        private:
            enum class TimerState : int
            {
                UNINITIALIZED, PAUSED, RUNNING
            };

            Units::MicroSecond m_begin_time           {0};
            Units::MicroSecond m_time_last_pause      {0};
            Units::MicroSecond m_pause_time_counter   {0};

            TimerState m_state = TimerState::UNINITIALIZED;

        public:

            explicit Timer() noexcept;

            void Restart()   noexcept;
            void Continue()  noexcept;
            void Pause()     noexcept;
            void Cancel()    noexcept;

            template <typename T> 
            requires Units::Is_Time_Unit<T>
            [[nodiscard]] inline T GetElapsedAndRestart() noexcept 
            {
                T elapsed (GetElapsed<T>());
                Restart();
                return elapsed;
            }

            template <typename T> 
            requires Units::Is_Time_Unit<T>
            [[nodiscard]] inline T GetElapsed() const noexcept 
            {
                const Units::MicroSecond time_now = Timer::GetTimeSinceEpoch<Units::MicroSecond>();

                if (m_state == TimerState::PAUSED) 
                    return Units::Convert<T>((time_now - m_begin_time) - m_pause_time_counter - (time_now - m_time_last_pause));

                else if (m_state == TimerState::RUNNING) 
                    return Units::Convert<T>((time_now - m_begin_time) - m_pause_time_counter);
                else
                    ENGINE_ASSERT(false && "At Timer::GetElapsed(): Trying to read state while uninitialized.");
                return T {};
            }

            template <typename T> 
            requires Units::Is_Time_Unit<T>
            [[nodiscard]] static inline T GetTimeSinceEpoch() noexcept 
            {
                const Units::MicroSecond time_now (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                return Units::Convert<T>(time_now);
            }
    };
}