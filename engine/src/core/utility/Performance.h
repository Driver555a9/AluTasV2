#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "core/utility/Units.h"
#include "core/utility/ScopedTimer.h"

#define ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME(msg) const ::CoreEngine::ScopedCallbackTimer timer_##__LINE__( \
    [](std::string&& message, ::CoreEngine::Units::MicroSecond ms) -> void { ::CoreEngine::PerFrameScopeTimes::CallScopeTime(std::move(message), ms); }, std::string(msg) ) 

#define ENGINE_PERFORMANCE_LOG_OCCURENCE(msg, count) ::CoreEngine::PerFrameOccurrenceCounter::CallOccurenceCounter(std::move(msg), count)

namespace CoreEngine
{  
    //////////////////////////////////////////////// 
    //--------- Scope times
    //////////////////////////////////////////////// 
    class PerFrameScopeTimes final
    {
    public:
        struct ScopeTimeData
        {
            std::string        m_message;
            Units::MicroSecond m_time {0};
            
            [[nodiscard]] constexpr inline std::string ToString() const
            {
                return m_message + " µs : " + std::to_string(m_time.Get());
            }
        };

        constexpr static inline void CallScopeTime(std::string&& message, Units::MicroSecond elapsed_millis) noexcept
        {
            auto it = std::find_if(s_scope_times.begin(), s_scope_times.end(), [&message](const ScopeTimeData& data) { return data.m_message == message; });

            if (it != s_scope_times.end())
            {
                it->m_time = elapsed_millis;
            }
            else
            {
                s_scope_times.emplace_back(std::move(message), elapsed_millis);
            }
        }

        constexpr static inline void ResetFrameScopeTimes() noexcept 
        { 
            s_scope_times.clear(); 
        }

        constexpr static inline void SortData() noexcept
        {
            std::sort(s_scope_times.begin(), s_scope_times.end(), [](const ScopeTimeData& a, const ScopeTimeData& b) -> bool { return a.m_time < b.m_time; } );
        }

        [[nodiscard]] constexpr static inline const std::vector<ScopeTimeData>& GetScopeTimeDataConstRef() noexcept 
        { 
            return s_scope_times; 
        }

    private:
        static inline std::vector<ScopeTimeData> s_scope_times;
        
    };


    //////////////////////////////////////////////// 
    //--------- Other important per frame data to track
    //////////////////////////////////////////////// 
    class PerFrameOccurrenceCounter final
    {
    public:
        struct OccurrenceCounterData
        {
            std::string        m_message;
            size_t             m_count {0};
            
            [[nodiscard]] constexpr inline std::string ToString() const
            {
                return m_message + std::to_string(m_count);
            }
        };

        constexpr static inline void CallOccurenceCounter(std::string&& message, const size_t count) noexcept
        {
            auto it = std::find_if(s_occurence_data.begin(), s_occurence_data.end(), [&message](const OccurrenceCounterData& data) { return data.m_message == message; });

            if (it != s_occurence_data.end())
            {
                it->m_count = count;
            }
            else
            {
                s_occurence_data.emplace_back(std::move(message), count);
            }
        }

        constexpr static inline void ResetFrameOccurenceCounts() noexcept 
        { 
            s_occurence_data.clear(); 
        }

        constexpr static inline void SortData() noexcept
        {
            std::sort(s_occurence_data.begin(), s_occurence_data.end(),
            [](const OccurrenceCounterData& a, const OccurrenceCounterData& b) -> bool { return a.m_count < b.m_count; } );
        }

        [[nodiscard]] constexpr static inline const std::vector<OccurrenceCounterData>& GetOccurrenceCounterDataConstRef() noexcept 
        { 
            return s_occurence_data; 
        }

    private:
        static inline std::vector<OccurrenceCounterData> s_occurence_data;
    };

}