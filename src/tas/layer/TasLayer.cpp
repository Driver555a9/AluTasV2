#include "tas/layer/TasLayer.h"

#include "core/event/ApplicationStateEvents.h"
#include "core/event/InputEvents.h"
#include "core/event/WindowEvents.h"
#include "core/event/EventDispatcher.h"
#include "core/utility/Assert.h"
#include "core/utility/Performance.h"
#include "core/scene/DummyCameraController.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/application/Application.h"

#include "tas/memory/MemoryUtility.h"
#include "tas/memory/MemoryRW.h"

#include "tas/globalstate/MemoryAddressState.h"
#include "tas/globalstate/GameState.h"

#include "tas/common/RacerState.h"
#include "tas/common/CameraState.h"

#include "tas/servicethreads/ReadCurrentStateService.h"
#include "tas/servicethreads/GameStateWatchdogService.h"
#include "tas/servicethreads/MemoryAddressUpdateService.h"
#include "tas/servicethreads/MouseInputService.h"
#include "tas/servicethreads/ReplayRecorderService.h"

#include "tas/layer/GuiStyle.h"
#include "tas/layer/CameraToolLayer.h"
#include "tas/layer/SpeedometerLayer.h"
#include "tas/layer/GhostToolLayer.h"

//ImGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/ImGuiFileDialog.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

namespace AsphaltTas
{
    TasLayer::TasLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        GameStateWatchdogService::LaunchThread();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        MemoryAddressUpdateService::LaunchThread();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ReadCurrentStateService::LaunchThread();

        //ReplayRecorder::LaunchRecordThread(CoreEngine::Units::Convert<CoreEngine::Units::MicroSecond>(CoreEngine::Units::MilliSecond(16)));
    }

    TasLayer::~TasLayer() noexcept 
    {
        GameState::OnInvalidateAllCaches();
    }

    void TasLayer::OnEvent(CoreEngine::Basic_Event& e) noexcept
    {
        CoreEngine::EventDispatcher disp(e);
        disp.Dispatch<CoreEngine::WindowCloseEvent>([]([[maybe_unused]] CoreEngine::WindowCloseEvent& e) -> bool {
            try 
            {
                MemoryRW::RestoreCameraUpdateCode(); ///Why is this needed? ~CameraToolLayer() should handle it! 
            } catch (...) {}
            CoreEngine::Application::Get()->Stop();
            return true;
        });
    }

    void TasLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
    {

    }

    void TasLayer::OnRender() noexcept
    {
        //OnRenderGhostExperimental();
    }

    void TasLayer::OnImGuiRender() noexcept
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

                ImGui::SameLine();
                EnterOrExitTool(GhostToolLayer::InstanceExists(), "Ghost Tool", &GhostToolLayer::DeleteInstance, &GhostToolLayer::CreateInstance);
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

                {
                    ImGui::TextUnformatted("Camera:");
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, CameraStateAddresses::AddressesAreValid() ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(CameraStateAddresses::ToString().c_str());
                }

                {
                    ImGui::TextUnformatted("Racer:");
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, RacerStateAddresses::AddressesAreValid() ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                    ImGui::TextUnformatted(RacerStateAddresses::ToString().c_str());
                }

                {
                    ImGui::TextUnformatted("Race Progress: ");
                    const auto DisplayAddress = [](uintptr_t (*func)(), const char* label) -> void {
                        const uintptr_t addrr = func();
                        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, addrr != INVALID_ADDRESS ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                        if (addrr != INVALID_ADDRESS)
                            ImGui::TextUnformatted(std::format("0x{:X} : {}", addrr, label).c_str());
                        else  
                            ImGui::TextUnformatted(std::format("Invalid : {}", label).c_str());
                    };

                    DisplayAddress(&RaceProgressStateAddresses::GetLapTimeAddress, "Lap Time");
                    DisplayAddress(&RaceProgressStateAddresses::GetRaceProgressAddress, "Progress");
                    DisplayAddress(&RaceProgressStateAddresses::GetCheckpointAddress, "Checkpoint");
                }

                try 
                {
                    ImGui::Text("Race Time: %.3f", CoreEngine::Units::Convert<CoreEngine::Units::Second>(ReadCurrentStateService::GetCurrentRaceProgressState()->m_lap_time).Get());
                } catch (...) {
                    ImGui::TextUnformatted("Race Time: -- Err --");
                }
                try 
                {
                    ImGui::Text("Race Progress: %.1f", ReadCurrentStateService::GetCurrentRaceProgressState()->m_race_progress_percentage);
                } catch (...) {
                    ImGui::TextUnformatted("Race Progress: -- Err --");
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

                    LogThreadStatus("Game State Watchdog   : ", GameStateWatchdogService::GetThreadIsRunning());
                    LogThreadStatus("Memory Address Update : ", MemoryAddressUpdateService::GetThreadIsRunning());
                    LogThreadStatus("Mouse Input Service   : ", MouseInputService::GetThreadIsRunning());
                    LogThreadStatus("Read Current State    : ", ReadCurrentStateService::GetThreadIsRunning());
                    LogThreadStatus("Replay Recorder       : ", ReplayRecorderService::GetThreadIsRunning());
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
}