#include "tas/common/Replay.h"

#include "core/utility/Assert.h"

namespace AsphaltTas
{
    const std::vector<Replay::Frame>& Replay::GetFrameVectorConstReference() const noexcept
    {
        return m_frames;
    }

    void Replay::IncrementFrameIndex(size_t count) noexcept
    {
        m_current_frame_index = std::min(m_current_frame_index + count, m_frames.size() - 1);
    }

    void Replay::GoToFirstFrameOverGivenLapTime(CoreEngine::Units::MicroSecond min_time) noexcept
    {
        if (m_frames.empty()) return;

        /////////////////////////////////
        // Go back if we're too far ahead
        /////////////////////////////////
        while (m_current_frame_index > 0 && m_frames[m_current_frame_index].m_race_progress_state.m_lap_time > min_time)
        {
            m_current_frame_index--;
        }

        /////////////////////////////////
        // Go forward if we're too far behind
        /////////////////////////////////
        while (m_current_frame_index < m_frames.size() - 1 && m_frames[m_current_frame_index].m_race_progress_state.m_lap_time < min_time)
        {
            m_current_frame_index++;
        }
    }

    void Replay::ResetFrameIndex() noexcept
    {
        m_current_frame_index = 0;
    }

    std::optional<Replay::Frame> Replay::GetCurrentFrame() const noexcept
    {
        if (m_frames.empty()) return std::nullopt;
        return m_frames[m_current_frame_index];
    }

    std::optional<Replay::Frame> Replay::GetLastFrame() const noexcept
    {
        if (m_frames.empty()) return std::nullopt;
        return m_frames.back();
    }

    size_t Replay::GetAmountFrames() const noexcept
    {
        return m_frames.size();
    }

    size_t Replay::GetCurrentIndex() const noexcept
    {
        return m_current_frame_index;
    }

    void Replay::ClearAllFrameData() noexcept
    {
        m_frames.clear();
        m_current_frame_index = 0;
    }
}