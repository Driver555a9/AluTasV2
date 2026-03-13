#pragma once

#include <string>
#include <sstream>
#include <iomanip>

#include "core/utility/Units.h" 

namespace AsphaltTas
{
    namespace Utility
    {
        [[nodiscard]] constexpr std::string TimeToFormatedString(CoreEngine::Units::MilliSecond time) noexcept
        {
            std::int64_t time_difference = time.Get();

            const int minutes = time_difference / 60'000;
            time_difference -= minutes * 60'000;
            const int seconds = time_difference / 1'000;
            time_difference -= seconds * 1'000;
            const int milis = static_cast<int>(time_difference);
            
            std::ostringstream result;
            result << std::setw(2) << std::setfill('0') << minutes << ":"
                   << std::setw(2) << std::setfill('0') << seconds << "."
                   << std::setw(3) << std::setfill('0') << milis;
            
            return result.str();
        }

        [[nodiscard]] constexpr float ConvertRealSpeedKmhToFakeSpeedKmh(float real_speed_kmh) noexcept
        {
            if (real_speed_kmh < 0) return 0.0f;
            if (real_speed_kmh <= 100) return real_speed_kmh;
            if (real_speed_kmh <= 200) return 1.35f * real_speed_kmh - 35;
            if (real_speed_kmh <= 300) return 1.85f * real_speed_kmh - 135;
            if (real_speed_kmh <= 350) return 2.6f * real_speed_kmh - 360;
            if (real_speed_kmh <= 400) return real_speed_kmh + 200;
            return 600;
        }
        
        [[nodiscard]] constexpr float ConvertFakeSpeedKmhToRealSpeedKmh(float fake_speed_kmh) noexcept
        {
            if (fake_speed_kmh < 0) return 0;
            if (fake_speed_kmh <= 100) return fake_speed_kmh;
            if (fake_speed_kmh <= 235) return (fake_speed_kmh + 35.0f) / 1.35;
            if (fake_speed_kmh <= 420) return (fake_speed_kmh + 135) / 1.85;
            if (fake_speed_kmh <= 550) return (fake_speed_kmh + 360) / 2.6;
            if (fake_speed_kmh <= 600) return fake_speed_kmh - 200;
            return 400;
        }

    }
}