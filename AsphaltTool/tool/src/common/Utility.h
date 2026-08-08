#pragma once

#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>

#include "core/utility/Units.h" 

namespace AsphaltTas
{
    namespace Utility
    {
        [[nodiscard]] inline std::string TimeToFormatedString(CoreEngine::Units::MilliSecond time) noexcept
        {
            std::int64_t t = time.Get();

            int minutes = t / 60000;
            t %= 60000;

            int seconds = t / 1000;
            int millis  = t % 1000;

            char buffer[16];
            std::snprintf(buffer, sizeof(buffer),"%02d:%02d.%03d", minutes, seconds, millis);

            return std::string(buffer);
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

        [[nodiscard]] inline bool IsValidFilename(std::string_view name) noexcept
        {
            static constexpr std::string_view invalid_chars = R"(\/:*?"<>|)";

            if (name.empty())
                return false;

            for (unsigned char c : name)
            {
                if (invalid_chars.find(static_cast<char>(c)) != std::string_view::npos || c < 0x20)
                {
                    return false;
                }
            }

            if (name.back() == '.' || name.back() == ' ')
                return false;

            std::string upper(name);
            std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            static constexpr std::array<std::string_view, 22> reserved = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
            };

            return std::find(reserved.begin(), reserved.end(), upper) == reserved.end();
        }
    }
}