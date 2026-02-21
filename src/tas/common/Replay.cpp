#include "tas/common/Replay.h"

#include "core/utility/Assert.h"

namespace AsphaltTas
{
    void Replay::IncrementFrameIndex(size_t count) noexcept
    {
        m_current_frame_index = std::min(m_current_frame_index + count, m_frames.size() - 1);
        
    }

    void Replay::IncrementToFirstFrameAfterGivenTime(CoreEngine::Units::MicroSecond min_time) noexcept
    {
        while (m_current_frame_index < m_frames.size())
        {
            if (m_frames[m_current_frame_index].m_time_since_begin >= min_time) { break; }
            m_current_frame_index++;
        }
    }

    void Replay::ResetFrameIndex() noexcept
    {
        m_current_frame_index = 0;
    }

    Replay::Frame Replay::GetCurrentFrame() const noexcept
    {
        return m_frames[m_current_frame_index];
    }

    Replay::Frame Replay::GetLastFrame() const noexcept
    {
        return m_frames.back();
    }

    size_t Replay::GetAmountFrames() const noexcept
    {
        return m_frames.size();
    }

    void Replay::ClearAllFrameData() noexcept
    {
        m_frames.clear();
        m_current_frame_index = 0;
    }
}