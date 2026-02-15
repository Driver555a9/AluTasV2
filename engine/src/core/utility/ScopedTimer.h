#pragma once

#include <functional>
#include <string>
#include <iostream>

#include "core/utility/Timer.h"
#include "core/utility/Units.h"

namespace CoreEngine
{
    class ScopedTimer : public Timer 
    {
    private:
        std::string m_message;

    public:
        explicit ScopedTimer(std::string message) noexcept : m_message(std::move(message)) {}

        ~ScopedTimer() noexcept 
        { 
            std::cout << "Scoped Timer: " << m_message << " : time micros: " << GetElapsed<Units::MicroSecond>().Get() << std::endl; 
        }
    };

    template <typename TFunc>
    requires (std::invocable<TFunc, std::string&&, Units::MicroSecond> )
    class ScopedCallbackTimer : public Timer 
    {   
    private:
        TFunc m_callback;
        std::string m_message;

    public:
        explicit ScopedCallbackTimer(TFunc&& callback, std::string message) noexcept 
        : m_callback(std::forward<TFunc>(callback)), m_message(std::move(message)) {}

        ~ScopedCallbackTimer() noexcept 
        { 
            std::invoke(m_callback, std::move(m_message), GetElapsed<Units::MicroSecond>()); 
        }
    };

}