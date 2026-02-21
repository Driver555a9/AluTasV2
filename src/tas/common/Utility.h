#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace AsphaltTas
{
    namespace Utility
    {
        [[nodiscard]] constexpr inline std::string TimeToFormatedString(size_t time) noexcept
        {
            size_t time_difference = time;

            int minutes = time / 60'000;
            time_difference -= minutes * 60'000;
            int seconds = time_difference / 1'000;
            time_difference -= seconds * 1'000;
            int milis = static_cast<int>(time_difference);
            
            std::ostringstream result;
            result << std::setw(2) << std::setfill('0') << minutes << ":"
                   << std::setw(2) << std::setfill('0') << seconds << "."
                   << std::setw(3) << std::setfill('0') << milis;
            
            return result.str();
        }

        [[nodiscard]] constexpr float ConvertRealSpeedKmhToFakeSpeedKmh(const float real_speed_kmh) noexcept
        {
            if (real_speed_kmh < 0) return 0.0f;
            if (real_speed_kmh <= 100) return real_speed_kmh;
            if (real_speed_kmh <= 200) return 1.35 * real_speed_kmh - 35;
            if (real_speed_kmh <= 300) return 1.85 * real_speed_kmh - 135;
            if (real_speed_kmh <= 350) return 2.6 * real_speed_kmh - 360;
            if (real_speed_kmh <= 400) return real_speed_kmh + 200;
            return 600;
        }
        
        [[nodiscard]] constexpr float ConvertFakeSpeedKmhToRealSpeedKmh(const float fake_speed_kmh) noexcept
        {
            if (fake_speed_kmh < 0) return 0.0f;
            if (fake_speed_kmh <= 100) return fake_speed_kmh;
            if (fake_speed_kmh <= 235) return (fake_speed_kmh + 35.0) / 1.35;
            if (fake_speed_kmh <= 420) return (fake_speed_kmh + 135) / 1.85;
            if (fake_speed_kmh <= 550) return (fake_speed_kmh + 360) / 2.6;
            if (fake_speed_kmh <= 600) return fake_speed_kmh - 200;
            return 400;
        }

    }
}