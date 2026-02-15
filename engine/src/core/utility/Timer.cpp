#include "core/utility/Timer.h"

//std
#include <chrono>
#include <iostream>

namespace CoreEngine
{
    Timer::Timer() noexcept
    { 
        Restart(); 
    }

    void Timer::Restart() noexcept
    {
        m_state = TimerState::RUNNING;
        m_begin_time = GetTimeSinceEpoch<Units::MicroSecond>();
        m_pause_time_counter = Units::MicroSecond(0);
    }

    void Timer::Continue() noexcept
    {

        if(m_state == TimerState::UNINITIALIZED) 
        {
            Restart();
            return;
        }

        if(m_state == TimerState::RUNNING) 
        {   
            return;
        }

        m_state = TimerState::RUNNING;
        m_pause_time_counter += GetTimeSinceEpoch<Units::MicroSecond>() - m_time_last_pause;
    }

    void Timer::Pause() noexcept
    {
        if(m_state != TimerState::RUNNING) 
        {
            return;
        }
        m_state = TimerState::PAUSED;
        m_time_last_pause = GetTimeSinceEpoch<Units::MicroSecond>();
    }

    void Timer::Cancel() noexcept
    {
        m_state = TimerState::UNINITIALIZED;
        m_begin_time = m_time_last_pause = m_pause_time_counter = Units::MicroSecond(0);
    }
}