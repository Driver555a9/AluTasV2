#pragma once

#include "tas/common/RacerState.h"
#include "tas/common/RaceProgressState.h"

#include "core/utility/Units.h"

#include <vector>
#include <optional>

namespace AsphaltTas
{
    class Replay
    {
    public:
        struct Frame
        {
            RacerState m_racer_state;
            RaceProgressState m_race_progress_state;
        };

        template <typename... Args>
        requires std::is_constructible_v<Frame, Args...>
        void EmplaceBackFrame(Args&&... args) noexcept
        {
            m_frames.emplace_back(std::forward<Args>(args)...);
        }

        template <typename... Args>
        requires std::is_constructible_v<std::vector<Frame>, Args...>
        inline void SetFrameData(Args&&... args) noexcept
        {
            m_frames = std::vector<Frame>(std::forward<Args>(args)...);
        }

        const std::vector<Frame>& GetFrameVectorConstReference() const noexcept;

        void IncrementFrameIndex(size_t count = 1) noexcept;
        void GoToFirstFrameOverGivenLapTime(CoreEngine::Units::MicroSecond min_time) noexcept;
        void ResetFrameIndex() noexcept;
        [[nodiscard]] std::optional<Frame> GetCurrentFrame() const noexcept;
        [[nodiscard]] std::optional<Frame> GetLastFrame() const noexcept;
        [[nodiscard]] size_t GetAmountFrames() const noexcept;
        [[nodiscard]] size_t GetCurrentIndex() const noexcept;

        void ClearAllFrameData() noexcept;

    private:
        std::vector<Frame> m_frames;
        size_t m_current_frame_index{0};
    };
}