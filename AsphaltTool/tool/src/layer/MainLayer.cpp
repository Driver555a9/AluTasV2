#include "layer/MainLayer.h"

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
#include "layer/TasInputLayer.h"

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

        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);

        if (! ImGui::Begin("Asphalt Tool", nullptr, flags ))
        {
            ImGui::End();
            return;
        }

        std::optional<ComDllOut::DllStateOut> dll_state_copy = AsphaltDllManager::GetDllStateOutCopy();
        
        { // Scope to delete scoped styles
        //////////////////////////////////////////////////////////
        // Features
        //////////////////////////////////////////////////////////
            if (ImGui::CollapsingHeader("Tools", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                const auto EnterOrExitTool = [](bool instance_exists, const char* label, void (*if_instance_exists)(), void (*if_instance_does_not_exist)() ) -> void 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, instance_exists ? GuiStyle::COLOR_RED : GuiStyle::COLOR_GREEN);
                    if (ImGui::Button((std::string(instance_exists ? "Exit " : "Enter ") + label).c_str()))
                    {
                        instance_exists ? if_instance_exists() : if_instance_does_not_exist();
                    }
                };

                EnterOrExitTool(CameraToolLayer::InstanceExists(), "Camera Tool", &CameraToolLayer::DeleteInstance, &CameraToolLayer::CreateInstance);

                ImGui::SameLine();
                EnterOrExitTool(SpeedometerLayer::InstanceExists(), "Speedometer", &SpeedometerLayer::DeleteInstance, &SpeedometerLayer::CreateInstance);

                //ImGui::SameLine();
                //EnterOrExitTool(GhostToolLayer::InstanceExists(), "Ghost Tool", &GhostToolLayer::DeleteInstance, &GhostToolLayer::CreateInstance);

                ImGui::SameLine();
                EnterOrExitTool(TasInputLayer::InstanceExists(), "Tas Inputs", &TasInputLayer::DeleteInstance, &TasInputLayer::CreateInstance);
            }   
            
        //////////////////////////////////////////////////////////
        // Address state
        //////////////////////////////////////////////////////////
            if (ImGui::CollapsingHeader("Addresses", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                switch (GameState::GetCurrentPlatform())
                {
                    case GameState::GamePlatform::MS:
                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                        ImGui::TextUnformatted("Platform: MS");
                        break;
                    }
                    case GameState::GamePlatform::STEAM: 
                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                        ImGui::TextUnformatted("Platform: Steam");
                        break;
                    }
                    default:
                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                        ImGui::TextUnformatted("Platform: NONE");
                    }
                }

                ImGui::SameLine();
                if (AsphaltDllManager::IsInjected())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                    ImGui::TextUnformatted("| DLL injected");
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted("| DLL not injected");
                }


                auto ToHex = [](uintptr_t addr) {
                    std::stringstream ss; 
                    ss << "0x" << std::uppercase << std::hex << addr;
                    return ss.str();
                };

                {
                    ImGui::TextUnformatted("Camera Base          :");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_camera_state_base_address ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_camera_state_base_address).c_str());
                }

                {
                    ImGui::TextUnformatted("Local Racer Base     :");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_local_racer_base_address ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_local_racer_base_address).c_str());
                }

                {
                    ImGui::Text("Nitro Bar Encrypted  :");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_nitro_bar_encrypted_address ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_nitro_bar_encrypted_address).c_str());
                }
                
                {
                    ImGui::Text("Stearing Struct Gear :");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_steering_struct_gear_address ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_steering_struct_gear_address).c_str());
                }

                {
                    ImGui::Text("Target Frame Interval:");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_game_target_fps_interval_address ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_game_target_fps_interval_address).c_str());
                }

                {
                    ImGui::Text("Respawn Func RCX Arg :");
                    ImGui::SameLine();
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, dll_state_copy.has_value() && dll_state_copy->m_resolved_addresses.m_respawn_func_spoofed_rcx_arg ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(ToHex(dll_state_copy->m_resolved_addresses.m_respawn_func_spoofed_rcx_arg).c_str());
                }
            }
            
            if (ImGui::CollapsingHeader("Tool Performance", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                {
                    const float fps = ImGui::GetIO().Framerate;

                    float t = (fps - 60.0f) / (240.0f - 60.0f);
                    t = std::clamp(t, 0.0f, 1.0f);

                    ImVec4 color;
                    if (t < 0.5f)
                    {
                        color = ImVec4(1.0f, t / 0.5f, 0.0f, 1.0f);
                    }
                    else
                    {
                        color = ImVec4(1.0f - ((t - 0.5f) / 0.5f), 1.0f, 0.0f, 1.0f);
                    }

                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, color);
                    ImGui::Text("Tool FPS: %.0f", fps);
                }

                bool vsync_is_on = CoreEngine::Application::Get()->GetVsyncIsOn();
                if (ImGui::Checkbox("VSync", &vsync_is_on))
                {
                    CoreEngine::Application::Get()->SetVsync(vsync_is_on);
                }

                if (ImGui::CollapsingHeader("Thread Status", ImGuiTreeNodeFlags_DefaultOpen ))
                {
                    auto LogThreadStatus = [](const char* name, bool is_active) -> void 
                    {
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, is_active ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                        ImGui::TextUnformatted(name);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(is_active ? "Active" : "Inactive");
                    };

                    LogThreadStatus("Dll Update Service    : ", DllStateUpdateService::GetThreadIsRunning());
                    LogThreadStatus("Game State Watchdog   : ", GameStateWatchdogService::GetThreadIsRunning());
                    LogThreadStatus("Mouse Input Service   : ", MouseInputService::GetThreadIsRunning());
                }

                if (ImGui::CollapsingHeader("Frame Times"))
                {
                    const std::vector<CoreEngine::PerFrameScopeTimes::ScopeTimeData>& scope_times = CoreEngine::PerFrameScopeTimes::GetScopeTimeDataConstRef();
                    for (const CoreEngine::PerFrameScopeTimes::ScopeTimeData& data : scope_times)
                    {
                        ImGui::TextUnformatted(data.ToString().c_str());
                    }
                }
            }

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
            .m_relative_size               = {460.0f / 1920.0f, 500.0f / 1080.0f},
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