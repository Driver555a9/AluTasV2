#include "tas/TasLayer.h"

#include "core/event/ApplicationStateEvents.h"
#include "core/event/InputEvents.h"
#include "core/event/WindowEvents.h"
#include "core/event/EventDispatcher.h"
#include "core/Utility/Assert.h"
#include "core/scene/DummyCameraController.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/application/ApplicationGlobalState.h"
#include "core/utility/Performance.h"

#include "tas/MemoryRW.h"
#include "tas/MemoryAddressState.h"
#include "tas/RacerState.h"
#include "tas/CameraState.h"
#include "tas/GameState.h"

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
    TasLayer::TasLayer() noexcept : m_pseudo_game_camera (glm::vec3(0.0f), CoreEngine::GlobalGet<CoreEngine::GlobalGet_AspectRatio>(), 50.0f, 0.1f)
    {
        m_pseudo_game_camera_controller.SetMoveSpeed(100.0f);
        m_pseudo_game_camera_controller.SetSensitivity(0.2f);

        GameState::LaunchDetectGameServiceThread();
        RacerStateAddresses::LaunchAddressUpdateServiceThread();
        CameraStateAddresses::LaunchAddressUpdateServiceThread();
    }

    TasLayer::~TasLayer() noexcept 
    {
        try { 
            MemoryRW::RestoreCameraUpdateCode(); 
        } catch(...) {}
        GameState::OnInvalidateAllCaches();
    }

    void TasLayer::OnEvent(CoreEngine::Basic_Event& e)
    {
        //CoreEngine::Freecam_3D_Layer::OnEvent(e);
    }

    void TasLayer::OnUpdate(CoreEngine::Units::MicroSecond dt)
    {
        if (IsInFreeFlight() && GameState::GetIsGameInForeground())
        {
        #ifdef _WIN32
            // Windows methods required (glfw key events only work if window focussed)
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_SPACE]            = GetAsyncKeyState(VK_SPACE) & 0x8000;
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_LEFT_CONTROL]     = GetAsyncKeyState(VK_LCONTROL) & 0x8000;
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_A]                = GetAsyncKeyState('A') & 0x8000;
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_D]                = GetAsyncKeyState('D') & 0x8000;
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_S]                = GetAsyncKeyState('S') & 0x8000;
            m_pseudo_game_input_state.m_key_is_pressed[GLFW_KEY_W]                = GetAsyncKeyState('W') & 0x8000;
            m_pseudo_game_input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_RIGHT] = true; // Hack to get CoreEngine::FreeCam_CameraController to always look around

            //////////////////////////////////////////////////////////
            // Calculate delta mouse movement from last update
            //////////////////////////////////////////////////////////
            POINT mouse_pos;
            GetCursorPos(&mouse_pos);

            if (!ImGui::GetIO().WantCaptureMouse)
            {
                if (m_pseudo_game_input_state.m_previous_mouse_pos.has_value())
                {
                    const glm::ivec2 prev = *m_pseudo_game_input_state.m_previous_mouse_pos;
                    m_pseudo_game_input_state.m_mouse_move_delta.x = mouse_pos.x - prev.x;
                    m_pseudo_game_input_state.m_mouse_move_delta.y = mouse_pos.y - prev.y;
                }
            }

            //////////////////////////////////////////////////////////
            // Center mouse in window
            //////////////////////////////////////////////////////////
            if (m_free_flight_recenter)
            {
                SetCursorPos(100, 100);
                m_pseudo_game_input_state.m_previous_mouse_pos = {100, 100};
            }
            else 
            {
                m_pseudo_game_input_state.m_previous_mouse_pos = { mouse_pos.x, mouse_pos.y };
            }

            m_pseudo_game_camera_controller.Update(m_pseudo_game_camera, m_pseudo_game_input_state, CoreEngine::Units::Convert<CoreEngine::Units::Second>(dt));

            CameraState out;
            out.m_position      = m_pseudo_game_camera.GetPosition();
            out.m_rotation      = m_pseudo_game_camera.GetRotation();
            out.m_fov_radians   = m_pseudo_game_camera.GetFovRad();

            try {
                MemoryRW::WriteCameraState(out, MemoryRW::IGNORE_FLAG_CAMERA::AspectRatio);
            } catch (...) {}

        #endif
        }

        //CoreEngine::Freecam_3D_Layer::OnUpdate(dt);
    }

    void TasLayer::OnRender()
    {
        //CoreEngine::Freecam_3D_Layer::OnRender();
    }

    void TasLayer::OnImGuiRender()
    {   
        ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);

        ImGui::Begin("Asphalt Tool", nullptr, ImGuiWindowFlags_NoNav );

        if (IsInFreeFlight())
        {
            OnImGuiRender_FreeFlightOptions();
        }
        else 
        {
            if (ImGui::Button("Enter Free Flight"))
            {
                try 
                {
                    CameraState camera_state_now;
                    camera_state_now = MemoryRW::ReadCameraState();
                    
                    m_pseudo_game_camera.SetPosition(camera_state_now.m_position);
                    m_pseudo_game_camera.SetRotation(camera_state_now.m_rotation);
                    m_pseudo_game_camera.SetFovRad(camera_state_now.m_fov_radians);

                    MemoryRW::DestroyCameraUpdateCode();
                } 
                catch (std::exception& e) { ENGINE_DEBUG_PRINT(e.what()); }
            }
        }

        if (ImGui::CollapsingHeader("Addresses", ImGuiTreeNodeFlags_DefaultOpen))
        {
            switch (GameState::GetCurrentPlatform())
            {
                case GameState::GamePlatform::MS:    ImGui::TextUnformatted("Platform: MS");    break;
                case GameState::GamePlatform::STEAM: ImGui::TextUnformatted("Platform: Steam"); break;
                default:                             ImGui::TextUnformatted("Platform: NONE");  break;
            }

            /*
            if (ImGui::Button("Generate"))
            {
                constexpr bool force_update = true;
                OnAttemptUpdateAddresses(force_update);
            } 

            if (ImGui::Checkbox("Automatically Update Addresses", &m_enable_auto_attempt_update_address))
            {
                if (m_enable_auto_attempt_update_address)
                {
                    RacerStateAddresses::LaunchAddressUpdateServiceThread();
                    CameraStateAddresses::LaunchAddressUpdateServiceThread();
                }
                else 
                {
                    RacerStateAddresses::StopAddressUpdateServiceThread();
                    CameraStateAddresses::StopAddressUpdateServiceThread();
                }
            }
            */

            ImGui::TextUnformatted("Camera:");
            ImGui::TextUnformatted(CameraStateAddresses::ToString().c_str());
            ImGui::TextUnformatted("Racer:");
            ImGui::TextUnformatted(RacerStateAddresses::ToString().c_str());
        }

        if (ImGui::CollapsingHeader("Tool Performance"))
        {
            ImGui::Text("Tool FPS: %.0f", ImGui::GetIO().Framerate);

            bool vsync_is_on = CoreEngine::GlobalGet<CoreEngine::GlobalGet_VsyncIsOn>();
            if (ImGui::Checkbox("VSync", &vsync_is_on))
            {
                CoreEngine::GlobalSet<CoreEngine::GlobalSet_VsyncIsOn>(vsync_is_on);
            }

            const std::vector<CoreEngine::PerFrameScopeTimes::ScopeTimeData>& scope_times = CoreEngine::PerFrameScopeTimes::GetScopeTimeDataConstRef();
            for (const CoreEngine::PerFrameScopeTimes::ScopeTimeData& data : scope_times)
            {
                ImGui::TextUnformatted(data.ToString().c_str());
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
        if (ImGui::Button("Exit"))
        {
            CoreEngine::GlobalSet<CoreEngine::GlobalSet_StopApplication>();
        }
        ImGui::PopStyleColor();

        ImGui::End();
    }

    void TasLayer::OnImGuiRender_FreeFlightOptions() noexcept
    {
        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

        ImGui::Begin("Free Flight", nullptr, ImGuiWindowFlags_NoNav );

        float velo = m_pseudo_game_camera_controller.GetMoveSpeed();
        if (ImGui::SliderFloat("Fly Speed", &velo, 0.1, 1000.0f, "%.3f"))
        {
            m_pseudo_game_camera_controller.SetMoveSpeed(velo);
        }

        float sensitivity = m_pseudo_game_camera_controller.GetSensitivity();
        if (ImGui::SliderFloat("Sensitivity", &sensitivity, 0.01f, 0.5f, "%.3f"))
        {
            m_pseudo_game_camera_controller.SetSensitivity(sensitivity);
        }

        float fov_deg = m_pseudo_game_camera.GetFovDeg();
        if (ImGui::SliderFloat("Fov", &fov_deg, 1.0f, 160.0f, "%.1f°"))
        {
            m_pseudo_game_camera.SetFovDeg(fov_deg);
        }

        ImGui::Checkbox("Center Mouse", &m_free_flight_recenter);

        const std::string pos_msg = "Position : " + CoreEngine::CommonUtility::GlmVec3ToString(m_pseudo_game_camera.GetPosition());
        ImGui::TextUnformatted(pos_msg.c_str());

        const std::string rot_msg = "Rotation : " + CoreEngine::CommonUtility::GlmQuatToString(m_pseudo_game_camera.GetRotation());
        ImGui::TextUnformatted(rot_msg.c_str());

        const std::string rot_euler_msg = "Rot Euler: " + CoreEngine::CommonUtility::GlmVec3ToString(glm::eulerAngles(m_pseudo_game_camera.GetRotation()));
        ImGui::TextUnformatted(rot_euler_msg.c_str());

        ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
        if (ImGui::Button("Exit"))
        {
            try 
            {
                MemoryRW::RestoreCameraUpdateCode();
            } 
            catch (std::exception& e)
            {
                ENGINE_DEBUG_PRINT(e.what());
            }
        }
        ImGui::PopStyleColor();

        ImGui::End();
    }

    bool TasLayer::OnAttemptUpdateAddresses(bool force_update_each) noexcept
    {
        bool success = true;

        if (! RacerStateAddresses::AddressesAreValid() || force_update_each)
        {
            try 
            {
                success &= RacerStateAddresses::UpdateAddresses();
            } catch (...) {}
        }

        if (! CameraStateAddresses::AddressesAreValid() || force_update_each)
        {
            try 
            {
                success &= CameraStateAddresses::UpdateAddresses();
            } catch (...) {}
        }

        return success;
    }

    bool TasLayer::IsInFreeFlight() const noexcept
    {
        return MemoryRW::CameraCodeIsDestroyed();
    }
}