#include "layer/TasInputLayer.h"

#include "Communication.h"
#include "common/Replay.h"
#include "common/Utility.h"
#include "core/application/Application.h"
#include "core/utility/Assert.h"
#include "imgui.h"
#include "layer/GuiStyle.h"
#include "globalstate/ReplayStateManager.h"
#include "globalstate/AsphaltDllManager.h"

#include <cstdint>
#include <filesystem>
#include <utility>


namespace AsphaltTas
{
    TasInputLayer::TasInputLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        s_instance = this;
    }

    TasInputLayer::~TasInputLayer() noexcept
    {
        s_instance = nullptr;
        ReplayStateManager::ClearQueuedReplay();
        ReplayStateManager::ClearInputCommandBuffer();
    }   
        
    void TasInputLayer::OnEvent([[maybe_unused]] CoreEngine::Basic_Event& e) noexcept
    {
        
    }

    void TasInputLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
    {
        
    }

    void TasInputLayer::OnRender() noexcept
    {

    }

    void TasInputLayer::OnImGuiRender() noexcept
    {
        ImVec2 display = ImGui::GetIO().DisplaySize;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(display);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                               | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);
        
        if (! ImGui::Begin("Tas Input", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        // Scope to delete scoped styles
        {
            if (!std::filesystem::exists(s_replay_folder_path))
            {
                std::filesystem::create_directories(s_replay_folder_path);
            }

            // ReplayStateManager is same thread as us, so no mutex needed
            const std::optional<ReplayStateManager::PlaybackSession>& active_replay = ReplayStateManager::GetQueuedPlaybackSessionConstRef();
            const std::optional<ComDllOut::DllStateOut> dll_out_copy = AsphaltDllManager::GetDllStateOutCopy();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // INSURE WE DO NOT CALL FUNCTIONS THAT TRY TO CLAIM DLL GENERAL CMD MUTEX IN THIS SCOPE!
                ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd_ref = AsphaltDllManager::GetDllGeneralCommandsInRef();

                // Recording, no changes
                if (dll_out_copy->m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE || active_replay.has_value())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_ORANGE);
                    ImGui::Text("Fixed Frame Interval Micros Locked: %u", general_cmd_ref->m_write_meta_data.m_fixed_frame_interval_micros);
                }
                else 
                {
                    // 4167 = 240fps; 33'332 = 30fps
                    ImGui::TextUnformatted("Fixed Frame Interval :");
                    ImGui::SameLine();
                    ImGui::SliderInt("##Fixed Frame Interval Micros", (int*)&general_cmd_ref->m_write_meta_data.m_fixed_frame_interval_micros, 4167, 33'332);
                }  

                ///////////////////// Desired frame interval / game target fps
                ImGui::TextUnformatted("Target Frame Interval:");
                ImGui::SameLine();
                ImGui::SliderInt("##Target Frame Interval", (int*)&general_cmd_ref->m_write_meta_data.m_game_target_fps_interval_micros, 1000, 33'332);

                ///////////////////// Replay Playback
                ImGui::TextUnformatted("Speed Up Replay      :");
                ImGui::SameLine();
                bool fast_forward = general_cmd_ref->m_write_meta_data.m_replay_speed_factor > 1;
                if (ImGui::Checkbox("##Replay fastforward", &fast_forward))
                {
                    general_cmd_ref->m_write_meta_data.m_replay_speed_factor = fast_forward ? 100'000 : 1;
                }

                ///////////////////// End of replay tick skip
                ImGui::TextUnformatted("Replay End Tick Skip :");
                ImGui::SameLine();
                ImGui::SliderInt("##Replay End Tick Skip", (int*)&general_cmd_ref->m_write_meta_data.m_on_replay_end_skip_tick_count, 0, 500);
                
                ImGui::Checkbox("Speed Up Race Intro", &general_cmd_ref->m_write_meta_data.m_speed_up_pre_race_cinematic);
                
                ///////////////////// LEGACY: Skip animations
                ImGui::SameLine();
                auto skip_flags = std::to_underlying(general_cmd_ref->m_write_meta_data.m_skip_animation_flags);

                bool skip_intro = skip_flags & std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_INTRO);
                if (ImGui::Checkbox("[Deprecated] Skip Intro", &skip_intro))
                {
                    if (skip_intro)
                        skip_flags |= (std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_INTRO) 
                                     | std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_COUNT_DOWN));
                    else
                        skip_flags &= ~(std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_INTRO)
                                      | std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_COUNT_DOWN));
                }
                general_cmd_ref->m_write_meta_data.m_skip_animation_flags = Communication::SkipAnimationFlags(skip_flags);

                ImGui::SameLine();
                ImGui::Checkbox("Speed Up GUI", &general_cmd_ref->m_write_meta_data.m_speed_up_gui_animations);

                ImGui::SameLine();
                ImGui::Checkbox("Transform Override", &m_use_transform_override_patch);

                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_BLUE);
                    if (ImGui::Button("Soft Reset Track"))
                    {
                        ENGINE_INFO_LOG("REQUEST BUTTON CLICK");
                        general_cmd_ref->m_write_meta_data.m_request_track_reset = true;
                    }
                }
            }
            if (ImGui::CollapsingHeader("Active Replay", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (active_replay.has_value())
                {   
                    ImGui::TextUnformatted(("Replay Name : " + active_replay->m_replay.GetName()).c_str());

                    {
                        const bool on = active_replay->m_replay.GetAmountFrames() > 0 ? 
                                        ! (active_replay->m_replay.GetFrameVectorConstReference()[0].m_replay_input.m_skip_override_flags 
                                        & ComDllIn::DllReplayInputIn::SkipOverride::TRANSFORM_FORCED) : m_use_transform_override_patch;

                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, on ? GuiStyle::COLOR_RED : GuiStyle::COLOR_GREEN);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(on ? " - Transform Override On" : " - Transform Override Off");
                    }
   
                    uint32_t last_tick = active_replay->m_final_tick;
                    ImGui::TextUnformatted("Target Tick :");
                    ImGui::SameLine();
                    if (ImGui::SliderInt("##Target Tick", (int*)&last_tick, 0, active_replay->m_replay.GetLastFrame()->m_replay_input.m_race_frame_tick))
                    {
                        ReplayStateManager::ChangeQueuedReplayTargetTick(last_tick);
                    }   

                    const bool playback_active = active_replay->m_replay.GetCurrentIndex() > 0;
                    if (playback_active)
                    {
                        ImGui::Text("Progress    : Tick %u/%u", std::min<uint32_t>(dll_out_copy->m_replay_inputs.m_race_frame_tick, active_replay->m_final_tick), active_replay->m_final_tick);
                    }
                    else 
                    {
                        ImGui::Text("Progress    : Tick %u/%u", 0, active_replay->m_final_tick);
                    }

                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                    if (ImGui::Button("Remove"))
                    {
                        ReplayStateManager::ClearQueuedReplay();
                    }
                }
                else
                {
                    ImGui::TextUnformatted("Replay Name : N/A");
                } 
            }

            if (ImGui::CollapsingHeader("Save Recorded Replays", ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::vector<Replay>& recorded_replays = ReplayStateManager::GetRecordedReplayListRef();

                std::vector<size_t> indices_to_be_deleted;
                for (size_t i{}; i < recorded_replays.size(); ++i)
                {
                    Replay& replay = recorded_replays[i]; 
                    ImGui::Text("Replay: %zu", i);
                    ImGui::SameLine();
                    
                    constexpr size_t max_name_len = 255;
                    std::string name = replay.GetName();
                    name.resize(max_name_len);

                    if (ImGui::InputText(("##Name" + std::to_string(i)).c_str(), name.data(), max_name_len + 1))
                    {
                        name.resize(strlen(name.c_str()));
                        replay.SetName(name);
                    }

                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                        ImGui::SameLine();
                        if (ImGui::Button((std::string("Save##NEW_REPLAY_") + std::to_string(i)).c_str()))
                        {
                            if (Utility::IsValidFilename(replay.GetName()) && Replay::SerializeReplayToFile(replay, s_replay_folder_path + replay.GetName() + ".REPLAY"))
                            {
                                indices_to_be_deleted.push_back(i);
                            }
                            else
                            {
                                ENGINE_ERROR_PRINT("Failed to save: " + replay.GetName() + " - verify valid filename.");
                            }
                        }

                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                        ImGui::SameLine();
                        if (ImGui::Button((std::string("Delete##NEW_REPLAY_") + std::to_string(i)).c_str()))
                        {
                            indices_to_be_deleted.push_back(i);
                        }
                    }
                }

                size_t z = indices_to_be_deleted.size();
                while (z-- > 0)
                {
                    size_t idx = indices_to_be_deleted[z];

                    if (idx < recorded_replays.size())
                    {
                        recorded_replays.erase(recorded_replays.begin() + idx);
                    }
                }

                ImGui::Text("Replay: %zu Recording... Tick: %u", recorded_replays.size(), dll_out_copy->m_replay_inputs.m_race_frame_tick);
            }

            if (ImGui::CollapsingHeader("Load Recorded Replay", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static std::vector<std::string> s_file_names;
                static std::filesystem::file_time_type s_last_cached_write_time;

                const auto ScanFolder = [&]()
                {
                    s_file_names.clear();

                    for (const auto& entry : std::filesystem::directory_iterator(s_replay_folder_path))
                    {
                        if (!entry.is_regular_file())
                        {
                            continue;
                        }

                        const std::filesystem::path& path = entry.path();

                        if (path.extension() == ".REPLAY")
                        {
                            s_file_names.push_back(path.filename().string());
                        }
                    }

                    std::sort(s_file_names.begin(), s_file_names.end());
                };

                const auto current_write_time = std::filesystem::last_write_time(s_replay_folder_path);

                if (current_write_time != s_last_cached_write_time)
                {
                    s_last_cached_write_time = current_write_time;
                    ScanFolder();
                }

                for (size_t i = 0; i < s_file_names.size(); ++i)
                {
                    const std::string& file = s_file_names[i];

                    ImGui::TextUnformatted(file.c_str());

                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                    
                    if (ImGui::Button((std::string("Load##RECORDED_REPLAY_") + std::to_string(i)).c_str()))
                    {
                        const std::string full_path = s_replay_folder_path + file;
                        Replay replay = Replay::DeserializeReplayFromFile(full_path);
                        if (! m_use_transform_override_patch)
                        {
                            for (auto& frame : replay.GetFrameVectorReference())
                            {
                                frame.m_replay_input.m_skip_override_flags |= ComDllIn::DllReplayInputIn::SkipOverride::TRANSFORM_FORCED;
                            }
                        }
                        ReplayStateManager::QueueReplay(replay, replay.GetLastFrame()->m_replay_input.m_race_frame_tick);
                    }

                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                    if (ImGui::Button((std::string("Delete##RECORDED_REPLAY_") + std::to_string(i)).c_str()))
                    {
                        const std::string full_path = s_replay_folder_path + file;
                        std::filesystem::remove(full_path);
                    }
                }
            }
        }

        ImGui::End();
    }

    ///////////////// Static functions
    void TasInputLayer::OnRaceStarted() noexcept
    {
        if (!s_instance) return;
        ReplayStateManager::OnRaceStarted();
    }

    void TasInputLayer::OnRaceEnded() noexcept
    {
        // Always allow race end cleanup
        ReplayStateManager::OnRaceEnded();
    }

    void TasInputLayer::OnDLLUpdate() noexcept
    {
        if (!s_instance) return;
        ReplayStateManager::OnUpdate();
    }

    void TasInputLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one TasInputLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "TasInput    ",
            .m_relative_size               = {800.0f / 1920.0f, 700.0f / 1080.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = false,
            .m_is_clickthrough             = false
        };
        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<TasInputLayer>(config);
    }

    bool TasInputLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void TasInputLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }
}