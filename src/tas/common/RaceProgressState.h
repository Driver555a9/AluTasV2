#pragma once

#include "core/utility/Units.h"

namespace AsphaltTas
{
    struct RaceProgressState
    {
        CoreEngine::Units::MicroSecond m_lap_time {0};
        float m_race_progress_percentage = 0.0f;
        int m_checkpoint = 0;

        [[nodiscard]] bool Equals(const RaceProgressState& other) const noexcept 
        {
            return m_lap_time == other.m_lap_time && m_race_progress_percentage == other.m_race_progress_percentage && m_checkpoint == other.m_checkpoint;
        }
    };
};