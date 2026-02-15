#pragma once

#include "tas/Replay.h"

#include "core/utility/Units.h"

#include <mutex>
#include <atomic>

namespace AsphaltTas
{
    class ReplayRecorder
    {
    public:
        explicit ReplayRecorder() noexcept = default;
        ~ReplayRecorder() noexcept; 

        void OnLaunchRecordThread(CoreEngine::Units::MicroSecond interval) noexcept;
        void OnStopRecordThread() noexcept;

        [[nodiscard]] size_t GetAmountRecordedFrames() noexcept;
        [[nodiscard]] Replay GetReplayCopy() noexcept;

        void ClearAllRecordedStates() noexcept;

        [[nodiscard]] bool GetThreadIsRunning() noexcept;

        ReplayRecorder(const ReplayRecorder&)           = delete;
        ReplayRecorder(ReplayRecorder&&)                = delete;
        ReplayRecorder& operator=(const ReplayRecorder) = delete;
        ReplayRecorder& operator=(ReplayRecorder&&)     = delete;

    private:
        void ThreadLoop(CoreEngine::Units::MicroSecond interval) noexcept;
        
        Replay m_replay;
        std::mutex m_replay_mutex;
        std::atomic<bool> m_stop_flag;
    };
}