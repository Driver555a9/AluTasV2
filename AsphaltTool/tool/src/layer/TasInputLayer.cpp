#include "layer/TasInputLayer.h"

#include "Communication.h"
#include "GuiStyle.h"
#include "common/Replay.h"
#include "common/Utility.h"
#include "core/application/Application.h"
#include "core/event/EventDispatcher.h"
#include "core/event/InputEvents.h"
#include "core/utility/Assert.h"
#include "core/utility/Units.h"
#include "imgui.h"
#include "layer/GuiStyle.h"
#include "globalstate/ReplayStateManager.h"
#include "globalstate/AsphaltDllManager.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <synchapi.h>
#include <utility>


namespace AsphaltTas
{
    TasInputLayer::TasInputLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        s_instance = this;
        OnLoadHotkeys(HOTKEY_DEF_FILE_NAME);
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
        for (Hotkey& key : m_hotkeys)
        {
            if (key.m_win32_key == 0) 
                continue;

            bool is_currently_down = (GetAsyncKeyState(key.m_win32_key) & 0x8000) != 0;

            if (is_currently_down && !key.m_was_pressed)
            {
                ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd_ref = AsphaltDllManager::GetDllGeneralCommandsInRef();
                
                switch (key.m_type)
                {
                    case Hotkey::Type::QUIT_RACE:
                    {
                        general_cmd_ref->m_write_meta_data.m_paused_menu_cmd = ComDllIn::PausedMenuCmd::QUIT;
                        break;
                    }   
                    case Hotkey::Type::RESTART_RACE:
                    {
                        general_cmd_ref->m_write_meta_data.m_paused_menu_cmd = ComDllIn::PausedMenuCmd::RESTART;
                        break;
                    }
                    case Hotkey::Type::NONE: 
                        break;
                }
            }

            key.m_was_pressed = is_currently_down;
        }
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
            if (!std::filesystem::exists(REPLAY_FOLDER_PATH))
            {
                std::filesystem::create_directories(REPLAY_FOLDER_PATH);
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
                ImGui::SliderInt("##Target Frame Interval", (int*)&general_cmd_ref->m_write_meta_data.m_target_frame_interval_micros, 1000, 33'332);

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
                ImGui::SliderInt("##Replay End Tick Skip", (int*)&general_cmd_ref->m_write_meta_data.m_on_replay_end_skip_tick_count, 0, 1000);
                
                ImGui::Checkbox("Speed Up Race Intro", &general_cmd_ref->m_write_meta_data.m_speed_up_pre_race_cinematic);
                
                ///////////////////// LEGACY: Skip animations
                ImGui::SameLine();
                ImGui::Checkbox("Speed Up GUI", &general_cmd_ref->m_write_meta_data.m_speed_up_gui_animations);

                ImGui::SameLine();
                ImGui::Checkbox("Transform Override", &m_use_transform_override_patch);

                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_BLUE);
                    if (ImGui::Button("Soft Reset Track"))
                    {
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

                {
                    if (!m_is_in_delete_all_process)
                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                        if (ImGui::Button(" Delete all replays ") && ! recorded_replays.empty())
                        {
                            m_is_in_delete_all_process = true;
                            m_delete_all_timer.Restart();
                        }
                    }
                    else 
                    {
                        {
                            PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                            if (ImGui::Button(" Cancel delete all  "))
                            {
                                m_is_in_delete_all_process = false;
                            }
                        }
                        const float secs_elapsed = m_delete_all_timer.GetElapsed<CoreEngine::Units::Second>().Get();
                        constexpr float SECS_UNTIL_OPTION_UNLOCKS = 3.0f;
                        ImGui::SameLine();
                        if (secs_elapsed < SECS_UNTIL_OPTION_UNLOCKS)
                        {
                            PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREY);
                            ImGui::Button(" Confirm delete all ");
                        }
                        else 
                        {
                            PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                            if (ImGui::Button(" Confirm delete all "))
                            {
                                indices_to_be_deleted = std::ranges::to<std::vector>(std::ranges::views::iota(0uz, recorded_replays.size()));
                                m_is_in_delete_all_process = false;
                            }
                        }
                    }
                }

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
                            if (Utility::IsValidFilename(replay.GetName()) && Replay::SerializeReplayToFile(replay, REPLAY_FOLDER_PATH + replay.GetName() + Communication::REPLAY_FILE_TYPE))
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
                enum class ReplaySortMode
                {
                    Name,
                    DateDesc,
                    DateAsc
                };

                struct ReplayFile
                {
                    ReplayFile(std::string name, std::filesystem::file_time_type write_time) noexcept : m_name(name), m_write_time(write_time)
                    {
                        const auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(m_write_time);
                        const std::time_t time = std::chrono::system_clock::to_time_t(system_time);

                        std::tm local_time{};
                        localtime_s(&local_time, &time);

                        std::ostringstream date_stream;
                        date_stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

                        m_write_time_str = date_stream.str();
                    };
                    std::string m_name;
                    std::filesystem::file_time_type m_write_time;
                    std::string m_write_time_str;
                };

                static std::vector<ReplayFile> s_files;
                static std::filesystem::file_time_type s_last_cached_write_time;
                static ReplaySortMode s_sort_mode = ReplaySortMode::Name;

                const auto SortFiles = [&]()
                {
                    switch (s_sort_mode)
                    {
                        case ReplaySortMode::Name:
                            std::sort(s_files.begin(), s_files.end(), [](const ReplayFile& a, const ReplayFile& b) { return a.m_name < b.m_name; });
                            break;

                        case ReplaySortMode::DateDesc:
                            std::sort(s_files.begin(), s_files.end(), [](const ReplayFile& a, const ReplayFile& b){ return a.m_write_time > b.m_write_time; });
                            break;
                        case ReplaySortMode::DateAsc:
                            std::sort(s_files.begin(), s_files.end(), [](const ReplayFile& a, const ReplayFile& b){ return a.m_write_time < b.m_write_time; });
                            break;
                        default:
                            ENGINE_ERROR_PRINT("Error! unkown replay sort mode: " << std::to_underlying(s_sort_mode));
                            std::sort(s_files.begin(), s_files.end(), [](const ReplayFile& a, const ReplayFile& b) { return a.m_name < b.m_name; });
                            break;
                    }
                };

                const auto ScanFolder = [&]()
                {
                    s_files.clear();

                    for (const auto& entry : std::filesystem::directory_iterator(REPLAY_FOLDER_PATH))
                    {
                        if (!entry.is_regular_file())
                        {
                            continue;
                        }

                        const std::filesystem::path& path = entry.path();

                        if (path.extension() != Communication::REPLAY_FILE_TYPE)
                        {
                            continue;
                        }

                        s_files.emplace_back(path.filename().string(), entry.last_write_time());
                    }

                    SortFiles();
                };

                const auto current_write_time = std::filesystem::last_write_time(REPLAY_FOLDER_PATH);

                if (current_write_time != s_last_cached_write_time)
                {
                    s_last_cached_write_time = current_write_time;
                    ScanFolder();
                }

                const auto GetModeName = [](ReplaySortMode mode) 
                {
                    switch (mode)
                    {
                        case ReplaySortMode::Name: return "Order by name";
                        case ReplaySortMode::DateDesc: return "Order by date Desc";
                        case ReplaySortMode::DateAsc: return "Order by date Asc";
                        default: return "Unkown";
                    }
                };

                ImGui::TextUnformatted("Sort:");
                ImGui::SameLine();

                if (ImGui::BeginCombo("##ReplaySort", GetModeName(s_sort_mode)))
                {
                    const auto DefineMode = [&GetModeName, &SortFiles](ReplaySortMode mode)
                    {
                        if (ImGui::Selectable(GetModeName(mode), s_sort_mode == mode))
                        {
                            s_sort_mode = mode;
                            SortFiles();
                        }
                    };

                    DefineMode(ReplaySortMode::Name);
                    DefineMode(ReplaySortMode::DateDesc);
                    DefineMode(ReplaySortMode::DateAsc);

                    ImGui::EndCombo();
                }

                for (size_t i = 0; i < s_files.size(); ++i)
                {
                    const ReplayFile& replay_file = s_files[i];
                    const std::string& file = replay_file.m_name;

                    ImGui::TextUnformatted(file.c_str());

                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);

                    if (ImGui::Button((std::string("Load##RECORDED_REPLAY_") + std::to_string(i)).c_str()))
                    {
                        const std::string full_path = REPLAY_FOLDER_PATH + file;
                        Replay replay = Replay::DeserializeReplayFromFile(full_path);

                        if (!m_use_transform_override_patch)
                        {
                            for (auto& frame : replay.GetFrameVectorReference())
                            {
                                frame.m_replay_input.m_skip_override_flags |=
                                    ComDllIn::DllReplayInputIn::SkipOverride::TRANSFORM_FORCED;
                            }
                        }

                        ReplayStateManager::QueueReplay(replay, replay.GetLastFrame()->m_replay_input.m_race_frame_tick);
                    }

                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);

                    if (ImGui::Button((std::string("Delete##RECORDED_REPLAY_") + std::to_string(i)).c_str()))
                    {
                        const std::string full_path = REPLAY_FOLDER_PATH + file;
                        Utility::SoftDelete(full_path);
                    }

                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", replay_file.m_write_time_str.c_str());
                }
            }
        }

        ImGui::End();
    }

    void TasInputLayer::OnLoadHotkeys(const std::string& path) noexcept
    {
        const auto CharToGLFWKey = [](char c) noexcept -> int
        {
            char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            if (upper >= 'A' && upper <= 'Z')
            {
                return GLFW_KEY_A + (upper - 'A');
            }

            if (upper >= '0' && upper <= '9')
            {
                return GLFW_KEY_0 + (upper - '0');
            }

            return GLFW_KEY_LAST;
        };

        const auto GlfwKeyToWin32 = [](int glfw_key) noexcept -> int
        {
            if (glfw_key >= GLFW_KEY_A && glfw_key <= GLFW_KEY_Z)
            {
                return 'A' + (glfw_key - GLFW_KEY_A);
            }
            if (glfw_key >= GLFW_KEY_0 && glfw_key <= GLFW_KEY_9)
            {
                return '0' + (glfw_key - GLFW_KEY_0);
            }
            return 0;
        };

        std::ifstream file(path);
        if (!file.is_open())
        {
            ENGINE_ERROR_PRINT("OnLoadHotkeys: Failed to open hotkey config file.");
            return;
        }

        m_hotkeys.clear();
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#') 
                continue;

            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) 
            {
                ENGINE_ERROR_PRINT("Hotkey line skipped - No : found");
                continue;
            }

            std::string action_str = line.substr(0, colon_pos);
            std::string val_str    = line.substr(colon_pos + 1);

            const auto trim = [](std::string& s) 
            {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            };

            trim(action_str);
            trim(val_str);

            std::transform(action_str.begin(), action_str.end(), action_str.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

            if (val_str.length() < 3 || val_str.front() != '"' || val_str.back() != '"')
            {
                ENGINE_ERROR_PRINT("Hotkey line skipped - malformed value. Value must be given like: \"n\"");
                continue;
            }

            char key_char = val_str[1];
            int glfw_key = CharToGLFWKey(key_char);
            if (glfw_key >= GLFW_KEY_LAST) 
            {
                ENGINE_ERROR_PRINT("Hotkey line skipped - value exceeds all GLFW keys: " << glfw_key);
                continue;
            }

            Hotkey hotkey;
            hotkey.m_gflw_key     = glfw_key;
            hotkey.m_win32_key    = GlfwKeyToWin32(glfw_key);
            hotkey.m_was_pressed = false;

            if (action_str == "restart")
            {
                hotkey.m_type = Hotkey::Type::RESTART_RACE;
                m_hotkeys.push_back(hotkey);
            }
            else if (action_str == "quit")
            {
                hotkey.m_type = Hotkey::Type::QUIT_RACE;
                m_hotkeys.push_back(hotkey);
            }
            else 
            {
                ENGINE_ERROR_PRINT("Hotkey line skipped - Action Type unkown: " << action_str);
                continue;
            }
        }
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