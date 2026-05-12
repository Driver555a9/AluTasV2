#include "globalstate/ReplayStateManager.h"
#include "AsphaltDllManager.h"
#include "Communication.h"
#include "common/Replay.h"

#include "core/utility/Assert.h"
#include "globalstate/AsphaltDllManager.h"

#include <cstdint>
#include <vector>

#include <optional>
#include <vector>

namespace AsphaltTas::ReplayStateManager
{
    namespace 
    {
        bool g_is_playback_active = false;
        bool g_is_in_race         = false;

        std::vector<Replay> g_recorded_replays;
        Replay g_current_recording;
        
        std::optional<PlaybackSession> g_queued_playback;

        void FinalizeRecording() noexcept
        {
            if (g_current_recording.GetAmountFrames() > 0) 
            {
                g_recorded_replays.push_back(std::move(g_current_recording));
            }
            g_current_recording = Replay();
        }

        void SetPlaybackActive(ComDllIn::DllGeneralCommandsIn& general_cmd) noexcept
        {
            general_cmd.m_write_meta_data.m_replay_mode = Communication::ReplayMode::ActiveBlockThread;
            g_is_playback_active = true;
        }

        void SetPlaybackInactive(ComDllIn::DllGeneralCommandsIn& general_cmd) noexcept
        {
            general_cmd.m_write_meta_data.m_replay_mode = Communication::ReplayMode::Inactive;
            g_is_playback_active = false;
        }
    }

    void ClearInputCmdBuffer() noexcept
    {
        ComSharedMem::GetSharedState()->m_dll_in_buffer_input_replay.Reset();
    }

    bool QueueReplay(const Replay& replay, uint32_t target_tick) noexcept 
    {
        if (IsPlaybackActive()) 
        {
            return false; 
        }
        
        g_queued_playback.emplace(PlaybackSession{replay, target_tick});
        return true;
    }

    bool ChangeQueuedReplayTargetTick(uint32_t target_tick) noexcept
    {
        if (IsPlaybackActive() || ! HasQueuedReplay())
        {
            return false;
        }

        g_queued_playback->m_final_tick = target_tick;
        return true;
    }

    bool HasQueuedReplay() noexcept
    {
        return g_queued_playback.has_value();
    }

    const std::optional<PlaybackSession>& GetQueuedPlaybackSessionConstRef() noexcept
    {
        return g_queued_playback;
    }

    void ClearQueuedReplay() noexcept 
    { 
        g_queued_playback.reset(); 
    }

    size_t GetCurrentRecordingAmountFrames() noexcept
    {
        return g_current_recording.GetAmountFrames();
    }

    bool IsPlaybackActive() noexcept 
    { 
        return g_is_playback_active; 
    }

    void OnRaceStarted() noexcept 
    {   
        ReplayStateManager::ClearInputCmdBuffer();
        g_current_recording.ClearAllFrameData();

        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd = AsphaltDllManager::GetDllGeneralCommandsInRef();

        if (g_queued_playback.has_value()) 
        {
            SetPlaybackActive(*general_cmd);

            g_queued_playback->m_replay.ResetFrameIndex();
        }
        else 
        {
            SetPlaybackInactive(*general_cmd);
        }

        g_current_recording.SetFrameIntervalMicros(AsphaltDllManager::GetDllStateOutCopy()->m_meta_data.m_fixed_frame_interval_micros);

        g_is_in_race = true;
    }

    void OnRaceEnded() noexcept 
    {
        if (g_is_in_race) 
        {
            FinalizeRecording();
        }
        
        ReplayStateManager::ClearInputCmdBuffer();

        g_is_in_race = false;
    }

    void OnUpdate() noexcept 
    {
        ///////////////////////////////////
        // Record new tick
        ///////////////////////////////////
        const std::optional<Communication::DllOut::DllStateOut> dll_out_state = AsphaltDllManager::GetDllStateOutCopy();

        if (! dll_out_state.has_value())
        {
            return;
        }

        if (g_is_in_race)
        {
            Replay::Frame new_frame;
            new_frame.m_replay_input.m_race_frame_tick                   = dll_out_state->m_replay_inputs.m_race_frame_tick;
            new_frame.m_replay_input.m_steer_value                       = dll_out_state->m_replay_inputs.m_steer_value;
            new_frame.m_replay_input.m_brake_value                       = dll_out_state->m_replay_inputs.m_brake_value;
            new_frame.m_replay_input.m_nitro_activation_count_this_frame = dll_out_state->m_replay_inputs.m_nitro_activation_count_this_frame;
            new_frame.m_replay_input.m_accelerator_value                 = dll_out_state->m_replay_inputs.m_accelerator_value;
            new_frame.m_replay_input.m_value_rbx_2228                    = dll_out_state->m_replay_inputs.m_value_rbx_2228;
            new_frame.m_replay_input.m_value_rbx_222C                    = dll_out_state->m_replay_inputs.m_value_rbx_222C;
            new_frame.m_replay_input.m_barrel_angular_velocities_vec3    = dll_out_state->m_replay_inputs.m_barrel_angular_velocities_vec3;
            new_frame.m_recorded_camera_state                            = dll_out_state->m_camera_state;
            new_frame.m_recorded_racer_state                             = dll_out_state->m_racer_state;

            g_current_recording.EmplaceBackFrame(new_frame);
        }

        ///////////////////////////////////
        // Playback update
        ///////////////////////////////////
        if (IsPlaybackActive() && g_queued_playback.has_value()) 
        {
            if (dll_out_state->m_replay_inputs.m_race_frame_tick >= g_queued_playback->m_final_tick) 
            {
                SetPlaybackInactive(*AsphaltDllManager::GetDllGeneralCommandsInRef());
                return;
            }

            while (const std::optional<Replay::Frame>& frame_opt = g_queued_playback->m_replay.GetCurrentFrame()) 
            {
                if (! frame_opt.has_value() || frame_opt->m_replay_input.m_race_frame_tick > g_queued_playback->m_final_tick)
                {
                    break;
                }

                if (ComSharedMem::GetSharedState()->m_dll_in_buffer_input_replay.TryPush(frame_opt->m_replay_input)) 
                {
                    g_queued_playback->m_replay.IncrementFrameIndex();
                } 
                else 
                {
                    break;
                }
            }
        }
    }

    std::vector<Replay>& GetRecordedReplayListRef() noexcept
    {
        return g_recorded_replays;
    }
}