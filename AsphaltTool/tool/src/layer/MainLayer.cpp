#include "layer/MainLayer.h"
#include <utility>
#include <array>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include "Communication.h"

#include "core/event/WindowEvents.h"
#include "core/event/EventDispatcher.h"
#include "core/utility/Assert.h"
#include "core/utility/Performance.h"
#include "core/application/Application.h"
#include "core/event/ApplicationStateEvents.h"

#include "globalstate/GameState.h"
#include "globalstate/AsphaltDllManager.h"

#include "servicethreads/GameStateWatchdogService.h"
#include "servicethreads/MouseInputService.h"
#include "servicethreads/DllStateUpdateService.h"

#include "layer/GuiStyle.h"
#include "layer/CameraToolLayer.h"
#include "layer/SpeedometerLayer.h"
#include "layer/GhostToolLayer.h"
#include "layer/DebugDrawStreamLayer.h"
#include "layer/TasInputLayer.h"
#include "layer/TrackViewerLayer.h"

//ImGUI
#include "imgui/imgui.h"
#include <winnt.h>

namespace AsphaltTas
{
    MainLayer::MainLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        GameStateWatchdogService::LaunchThread();
        DllStateUpdateService::LaunchThread();

        s_instance = this;
    }

    MainLayer::~MainLayer() noexcept 
    {
        s_instance = nullptr;
    }

    void MainLayer::OnEvent(CoreEngine::Basic_Event& e) noexcept
    {
        CoreEngine::EventDispatcher disp(e);
        disp.Dispatch<CoreEngine::WindowCloseEvent>([]([[maybe_unused]] CoreEngine::WindowCloseEvent& e) -> bool 
        {
            CoreEngine::Application::Get()->Stop();
            return true;
        });
        disp.Dispatch<CoreEngine::ApplicationShutdownEvent>([]([[maybe_unused]] CoreEngine::ApplicationShutdownEvent& e) -> bool 
        {
            GameState::OnShutdownCleanup();
            return true;
        });
    }

    void MainLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
    {
        if (! AsphaltDllManager::IsInjected() && GameState::GetHasValidCurrentPlatform())
        {
            try 
            {
                AsphaltDllManager::InjectIntoGame();
            } catch (...) {}
        }
    }

    void MainLayer::OnRender() noexcept
    {

    }

    void MainLayer::OnImGuiRender() noexcept
    {   
        ImVec2 display = ImGui::GetIO().DisplaySize;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(display);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                               | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_WindowBg, GuiStyle::COLOR_BLACK);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

        if (! ImGui::Begin("Asphalt Tool", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        std::optional<ComDllOut::DllStateOut> dll_state_copy = AsphaltDllManager::GetDllStateOutCopy();

        if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Tools"))
            {
                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                const auto DrawToolButton = [](bool instance_exists, const char* label, void (*if_instance_exists)(), void (*if_instance_does_not_exist)()) -> void 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, instance_exists ? GuiStyle::COLOR_RED : GuiStyle::COLOR_GREEN);
                    std::string button_text = (instance_exists ? "Close " : "Launch ") + std::string(label);
                    if (ImGui::Button(button_text.c_str(), ImVec2(180.0f, 38.0f)))
                    {
                        instance_exists ? if_instance_exists() : if_instance_does_not_exist();
                    }
                };

                if (ImGui::BeginTable("ToolsGrid", 2, ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    DrawToolButton(TasInputLayer::InstanceExists(), "TAS Inputs", &TasInputLayer::DeleteInstance, &TasInputLayer::CreateInstance);

                    ImGui::TableSetColumnIndex(1);
                    DrawToolButton(TrackViewerLayer::InstanceExists(), "Track Viewer", &TrackViewerLayer::DeleteInstance, &TrackViewerLayer::CreateInstance);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    DrawToolButton(CameraToolLayer::InstanceExists(), "Camera Tool", &CameraToolLayer::DeleteInstance, &CameraToolLayer::CreateInstance);

                    ImGui::TableSetColumnIndex(1);
                    //DrawToolButton(SpeedometerLayer::InstanceExists(), "Speedometer", &SpeedometerLayer::DeleteInstance, &SpeedometerLayer::CreateInstance);
                    DrawToolButton(DebugDrawStreamLayer::InstanceExists(), "Debug Draw", &DebugDrawStreamLayer::DeleteInstance, &DebugDrawStreamLayer::CreateInstance);

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Addresses"))
            {
                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                ImGui::TextUnformatted("Platform:");
                ImGui::SameLine();
                const auto platform = GameState::GetCurrentPlatform();
                if (platform == GameState::GamePlatform::STEAM)
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                    ImGui::TextUnformatted("Steam");
                }
                else if (platform == GameState::GamePlatform::MS)
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                    ImGui::TextUnformatted("MS Store");
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted("None");
                }

                ImGui::SameLine(220.0f);
                ImGui::TextUnformatted("DLL Status:");
                ImGui::SameLine();
                if (AsphaltDllManager::IsInjected())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                    ImGui::TextUnformatted("Injected");
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted("Not Injected");
                }

                ImGui::Separator();

                auto ToHex = [](uintptr_t addr) {
                    std::stringstream ss; 
                    ss << "0x" << std::uppercase << std::hex << addr;
                    return ss.str();
                };

                struct AddressEntry { const char* name; uintptr_t addr; };
                std::array<AddressEntry, 12> entries;
                if (dll_state_copy.has_value())
                {
                    entries[0] = { "Camera Base", dll_state_copy->m_resolved_addresses.m_camera_state_base_address };
                    entries[1] = { "Local Racer Base", dll_state_copy->m_resolved_addresses.m_local_racer_base_address };
                    entries[2] = { "Nitro Bar Encrypted", dll_state_copy->m_resolved_addresses.m_nitro_bar_encrypted_address };
                    entries[3] = { "Steering Struct Base", dll_state_copy->m_resolved_addresses.m_steering_struct_base_address };
                    entries[4] = { "Target Frame Interval", dll_state_copy->m_resolved_addresses.m_game_target_fps_interval_address };
                    entries[5] = { "Respawn func RCX Arg", dll_state_copy->m_resolved_addresses.m_respawn_func_spoofed_rcx_arg };
                    entries[6] = { "Target Fps Interval", dll_state_copy->m_resolved_addresses.m_game_target_fps_interval_address };
                    entries[7] = { "Brake func RCX Arg", dll_state_copy->m_resolved_addresses.m_brake_func_spoofed_rcx_arg };
                    entries[8] = { "Nitro func RCX Arg", dll_state_copy->m_resolved_addresses.m_nitro_func_spoofed_rcx_arg };
                    entries[9] = { "Dynamics World", dll_state_copy->m_resolved_addresses.m_discrete_dynamics_world_instance_address };
                    entries[10] = {"World Wrapper", dll_state_copy->m_resolved_addresses.m_physics_world_wrapper_address };
                    entries[11] = { "Physics Context", dll_state_copy->m_resolved_addresses.m_physics_context_address };
                }
                else 
                {
                    entries[0] = { "Camera Base", 0};
                    entries[1] = { "Local Racer Base", 0};
                    entries[2] = { "Nitro Bar Encrypted", 0};
                    entries[3] = { "Steering Struct Base", 0};
                    entries[4] = { "Target Frame Interval", 0 };
                    entries[5] = { "Respawn func RCX Arg", 0 };
                    entries[6] = { "Target Fps Interval", 0};
                    entries[7] = { "Brake func RCX Arg", 0};
                    entries[8] = { "Nitro func RCX Arg", 0};
                    entries[9] = { "Dynamics World", 0};
                    entries[10] = {"World Wrapper", 0};
                    entries[11] = { "Physics Context", 0};
                }

                if (ImGui::BeginTable("AddressesTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
                {
                    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                    ImGui::TableSetupColumn("Memory Address", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (const auto& entry : entries)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(entry.name);

                        ImGui::TableSetColumnIndex(1);
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, entry.addr != 0 ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                        ImGui::TextUnformatted(ToHex(entry.addr).c_str());
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance"))
            {
                ImGui::Dummy(ImVec2(0.0f, 4.0f));

                const float fps = ImGui::GetIO().Framerate;
                float t = std::clamp((fps - 60.0f) / (240.0f - 60.0f), 0.0f, 1.0f);
                ImVec4 color = (t < 0.5f) ? ImVec4(1.0f, t / 0.5f, 0.0f, 1.0f) : ImVec4(1.0f - ((t - 0.5f) / 0.5f), 1.0f, 0.0f, 1.0f);

                ImGui::TextUnformatted("Tool FPS:");
                ImGui::SameLine();
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, color);
                ImGui::Text("%.0f", fps);

                ImGui::SameLine(180.0f);
                bool vsync_is_on = CoreEngine::Application::Get()->GetVsyncIsOn();
                if (ImGui::Checkbox("VSync", &vsync_is_on))
                {
                    CoreEngine::Application::Get()->SetVsync(vsync_is_on);
                }

                ImGui::Separator();

                if (ImGui::CollapsingHeader("Thread Status", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::BeginTable("ThreadTable", 2, ImGuiTableFlags_BordersInnerH))
                    {
                        ImGui::TableSetupColumn("Service", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);

                        auto LogThreadRow = [](const char* name, bool active) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(name);
                            ImGui::TableSetColumnIndex(1);
                            PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, active ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                            ImGui::TextUnformatted(active ? "Active" : "Inactive");
                        };

                        LogThreadRow("DLL Update Service", DllStateUpdateService::GetThreadIsRunning());
                        LogThreadRow("Game State Watchdog", GameStateWatchdogService::GetThreadIsRunning());
                        LogThreadRow("Mouse Input Service", MouseInputService::GetThreadIsRunning());

                        ImGui::EndTable();
                    }
                }

                if (ImGui::CollapsingHeader("Frame Times"))
                {
                    const std::vector<CoreEngine::PerFrameScopeTimes::ScopeTimeData>& scope_times = CoreEngine::PerFrameScopeTimes::GetScopeTimeDataConstRef();
                    for (const CoreEngine::PerFrameScopeTimes::ScopeTimeData& data : scope_times)
                    {
                        ImGui::TextUnformatted(data.ToString().c_str());
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void MainLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one TasInputLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig window_config
        {
            .m_title                       = "Asphalt Tool",
            .m_relative_size               = {400.0f / 1920.0f, 500.0f / 1080.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = false,
            .m_is_clickthrough             = false
        };
        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<MainLayer>(window_config);
    }

    bool MainLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void MainLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }
}