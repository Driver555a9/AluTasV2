#include "tas/layers/TasLayer.h"

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

#include "tas/layers/GuiStyle.h"
#include "tas/layers/CameraToolLayer.h"
#include "tas/layers/SpeedometerLayer.h"

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
        MemoryAddressUpdateService::LaunchThread();
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
        disp.Dispatch<CoreEngine::WindowCloseEvent>([](CoreEngine::WindowCloseEvent& e) -> bool {
            try 
            {
                MemoryRW::RestoreCameraUpdateCode(); ///Why is this needed? ~CameraToolLayer() should handle it! 
            } catch (...) {}
            CoreEngine::Application::Get()->Stop();
            return true;
        });
    }

    void TasLayer::OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept
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
                if (CameraToolLayer::InstanceExists())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                    if (ImGui::Button("Exit Freeflight"))
                    {
                        CameraToolLayer::DeleteInstance();
                    }
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                    if (ImGui::Button("Enter Freeflight"))
                    {
                        CameraToolLayer::CreateInstance();
                    }
                }

                ImGui::SameLine();
                if (SpeedometerLayer::InstanceExists())
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_RED);
                    if (ImGui::Button("Exit Speedometer"))
                    {
                        SpeedometerLayer::DeleteInstance();
                    }
                }
                else 
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                    if (ImGui::Button("Enter Speedometer"))
                    {
                        SpeedometerLayer::CreateInstance();
                    }
                }
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

                ImGui::TextUnformatted("Camera:");
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, CameraStateAddresses::AddressesAreValid() ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                ImGui::TextUnformatted(CameraStateAddresses::ToString().c_str());

                ImGui::TextUnformatted("Racer:");
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, RacerStateAddresses::AddressesAreValid() ? GuiStyle::COLOR_GREEN : GuiStyle::COLOR_RED);
                ImGui::TextUnformatted(RacerStateAddresses::ToString().c_str());
            }
            
            if (ImGui::CollapsingHeader("Tool Performance", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
            {
                ImGui::Text("Tool FPS: %.0f", ImGui::GetIO().Framerate);

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

                if (ImGui::CollapsingHeader("Frame Times", ImGuiTreeNodeFlags_DefaultOpen ))
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

    void TasLayer::OnRenderGhostExperimental() noexcept
    {
        static CoreEngine::CameraReverseZ s_pseudo_game_camera (glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 55.0f, 0.1f);
        s_pseudo_game_camera.SetAspectRatio(CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio());

        static CoreEngine::Scene3D s_scene {};
        static CoreEngine::IndirectDraw3D_RenderPipeline s_render_pipeline {};

        if (s_scene.GetSceneObjectsRef().empty())
        {
            CoreEngine::Scene3D_ObjectBuilder builder = s_scene.CreateObjectBuilder();
            builder.RenderModel_SetFromPath("resources/obj/h2.glb", glm::vec3(1.0f));
            builder.SetPhysicsObjectType(CoreEngine::PhysicsObjectType::NONE);
            builder.SetName("Ghost");
            s_scene.AddObjectFromBuilder(std::move(builder));

            s_scene.EmplaceLightSource(glm::vec3(0), glm::vec3(1), 60.0f, CoreEngine::Light::LIGHT_MODE::DIRECT_LIGHT);

            s_render_pipeline.SetSceneData(s_scene.GetRenderModelVector(), s_scene.GetLightVectorConstRef());
        }

        static CoreEngine::Timer timer;
        
        if (timer.GetElapsed<CoreEngine::Units::MilliSecond>() < CoreEngine::Units::MilliSecond(16)) 
        {
            s_render_pipeline.Render();
            return;
        }
        timer.Restart();

        std::vector<std::unique_ptr<CoreEngine::Scene3D_SceneObject>>& objects = s_scene.GetSceneObjectsRef();

        CameraState camera_state_now;
        RacerState racer_state;
        
        try {
            camera_state_now = MemoryRW::ReadCameraState();
            racer_state      = MemoryRW::ReadRacerState();
        } catch (...) { s_render_pipeline.Render(); }

        objects[0]->SetPosition(racer_state.GetExtractedPosition());
        objects[0]->SetRotation(racer_state.GetExtractedRotation());

        s_pseudo_game_camera.SetPosition(camera_state_now.m_position);
        s_pseudo_game_camera.SetRotation(camera_state_now.m_rotation);
        s_pseudo_game_camera.SetFovRad(camera_state_now.m_fov_radians);

        s_render_pipeline.SetSceneData(s_scene.GetRenderModelVector(),
         {CoreEngine::Light{racer_state.GetExtractedPosition(), glm::vec3(1), 350.0f, CoreEngine::Light::DIRECT_LIGHT}});

        s_render_pipeline.SetCameraData(s_pseudo_game_camera.CalculateCameraMatrix(), s_pseudo_game_camera.GetPosition());

        s_render_pipeline.Render();
    }

}