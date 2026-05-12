#pragma once

#include "Communication.h"
#include "common/Replay.h"

namespace ComSharedMem = Communication::SharedMemory;
namespace ComDllIn     = Communication::DllIn;
namespace ComDllOut    = Communication::DllOut;

namespace AsphaltTas
{
    namespace ReplayStateManager
    {
        struct PlaybackSession 
        {
            Replay m_replay;
            uint32_t m_final_tick;
        };

        void ClearInputCmdBuffer() noexcept;

        bool QueueReplay(const Replay& replay, uint32_t target_tick) noexcept;
        bool ChangeQueuedReplayTargetTick(uint32_t target_tick) noexcept;
        bool HasQueuedReplay() noexcept;
        const std::optional<PlaybackSession>& GetQueuedPlaybackSessionConstRef() noexcept;
        void ClearQueuedReplay() noexcept;

        size_t GetCurrentRecordingAmountFrames() noexcept;

        bool IsPlaybackActive() noexcept;

        void OnRaceStarted() noexcept;
        void OnRaceEnded() noexcept;

        void OnUpdate() noexcept;

        std::vector<Replay>& GetRecordedReplayListRef() noexcept;
    }
}