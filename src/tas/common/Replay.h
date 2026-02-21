#pragma once

#include "tas/common/RacerState.h"

#include "core/utility/Units.h"

#include <vector>

namespace AsphaltTas
{
    class Replay
    {
    public:
        struct Frame
        {
            RacerState m_racer_state;
            CoreEngine::Units::MicroSecond m_time_since_begin;
        };

        template <typename... Args>
        requires std::is_constructible_v<Frame, Args...>
        void EmplaceBackFrame(Args&&... args) noexcept
        {
            m_frames.emplace_back(std::forward<Args>(args)...);
        }

        void IncrementFrameIndex(size_t count = 1) noexcept;
        void IncrementToFirstFrameAfterGivenTime(CoreEngine::Units::MicroSecond min_time) noexcept;
        void ResetFrameIndex() noexcept;
        [[nodiscard]] Frame GetCurrentFrame() const noexcept;
        [[nodiscard]] Frame GetLastFrame() const noexcept;
        [[nodiscard]] size_t GetAmountFrames() const noexcept;

        void ClearAllFrameData() noexcept;

    private:
        std::vector<Frame> m_frames;
        size_t m_current_frame_index{0};
    };
}