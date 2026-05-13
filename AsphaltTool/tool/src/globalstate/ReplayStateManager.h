#pragma once

#include "Communication.h"
#include "common/Replay.h"
#include <cstdint>

namespace AsphaltTas
{
    namespace ReplayStateManager
    {
        struct PlaybackSession 
        {
            Replay m_replay;
            uint32_t m_final_tick;
        };

        void ClearInputCommandBuffer() noexcept;

        bool QueueReplay(const Replay& replay, uint32_t target_tick) noexcept;
        bool ChangeQueuedReplayTargetTick(uint32_t target_tick) noexcept;
        [[nodiscard]] bool HasQueuedReplay() noexcept;
        [[nodiscard]] const std::optional<PlaybackSession>& GetQueuedPlaybackSessionConstRef() noexcept;
        void ClearQueuedReplay() noexcept;

        void OnRaceStarted() noexcept;
        void OnRaceEnded() noexcept;
        void OnUpdate() noexcept;

        [[nodiscard]] std::vector<Replay>& GetRecordedReplayListRef() noexcept;
    }
}