#include "layer/CameraToolLayer.h"

#include "Communication.h"
#include "core/utility/Assert.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/application/Application.h"

#include "core/utility/CommonUtility.h"
#include "globalstate/AsphaltDllManager.h"

#include "common/RacerState.h"
#include "common/CameraState.h"

#include "servicethreads/MouseInputService.h"

#include "layer/GuiStyle.h"

//ImGUI
#include "imgui/imgui.h"
#include <exception>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <algorithm>

namespace AsphaltTas
{
    CameraToolLayer::CameraToolLayer(CoreEngine::Window::Handle handle) noexcept : Basic_Layer(handle),
    m_free_cam_pseudo_camera (glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 55.0f, 0.1f),
    m_orbital_cam_pseudo_camera (glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 55.0f, 0.1f),
    m_front_car_cam_pseudo_camera (glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 55.0f, 0.1f)
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one FreeFlightLayer active at one time.");

        s_instance = this;

        s_free_cam_controller.SetMoveSpeed(100.0f);
        s_free_cam_controller.SetSensitivity(0.2f);

        s_orbital_cam_controller.SetDistance(5.0f);
        s_orbital_cam_controller.SetSensitivity(0.2f);

        try 
        {
            CameraState camera_state_now (AsphaltDllManager::GetDllStateOutCopy()->m_camera_state);
            
            m_free_cam_pseudo_camera.SetPosition(camera_state_now.GetPositionOpenGL_XYZ());
            m_free_cam_pseudo_camera.SetRotation(camera_state_now.GetRotationOpenGL_WXYZ());
           
            m_orbital_cam_pseudo_camera.SetPosition(camera_state_now.GetPositionOpenGL_XYZ());
            m_orbital_cam_pseudo_camera.SetRotation(camera_state_now.GetRotationOpenGL_WXYZ());

            m_front_car_cam_pseudo_camera.SetPosition(camera_state_now.GetPositionOpenGL_XYZ());
            m_front_car_cam_pseudo_camera.SetRotation(camera_state_now.GetRotationOpenGL_WXYZ());

            MouseInputService::LaunchThread();
        } 
        catch (std::exception& e) 
        { 
            DeleteInstance();
            ENGINE_ERROR_PRINT("Failed to enter free flight: " << e.what()); 
        }
    }

    CameraToolLayer::~CameraToolLayer() noexcept
    {
        MouseInputService::StopThread();

        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd_ref_lock = AsphaltDllManager::GetDllGeneralCommandsInRef(); 

        general_cmd_ref_lock->m_write_camera_state.m_continuous_override_on_flags = 0;
        general_cmd_ref_lock->m_write_camera_state.m_command_type = ComDllIn::CommandType::ExecuteCommand;

        general_cmd_ref_lock->m_write_meta_data.m_hide_gui = false;
        general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;

        s_instance = nullptr;
    } 

    void CameraToolLayer::OnEvent([[maybe_unused]] CoreEngine::Basic_Event& e) noexcept 
    {

    }

    void CameraToolLayer::OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept 
    {
        CoreEngine::InputState input_state;
    #ifdef _WIN32
        // Windows methods required (glfw key events only work if window focussed)
        input_state.m_key_is_pressed[GLFW_KEY_SPACE]            = GetAsyncKeyState(VK_SPACE) & 0x8000;
        input_state.m_key_is_pressed[GLFW_KEY_LEFT_CONTROL]     = GetAsyncKeyState(VK_LCONTROL) & 0x8000;
        input_state.m_key_is_pressed[GLFW_KEY_A]                = GetAsyncKeyState('A') & 0x8000;
        input_state.m_key_is_pressed[GLFW_KEY_D]                = GetAsyncKeyState('D') & 0x8000;
        input_state.m_key_is_pressed[GLFW_KEY_S]                = GetAsyncKeyState('S') & 0x8000;
        input_state.m_key_is_pressed[GLFW_KEY_W]                = GetAsyncKeyState('W') & 0x8000;
        input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_RIGHT] = true; // Hack to get Camera Controllers to always look around
    #endif
        //////////////////////////////////////////////////////////
        // Calculate delta mouse movement from last update
        //////////////////////////////////////////////////////////
        input_state.m_mouse_move_delta = MouseInputService::GetMouseDeltaMovementAndReset();

        CameraState out;

        std::optional<ComDllOut::DllStateOut> dll_state_out_opt_copy = AsphaltDllManager::GetDllStateOutCopy();

        if (s_current_controller_type == CameraControllerType::FREE_CAM)
        {
            float speed            = s_free_cam_controller.GetMoveSpeed();
            const float dt_secs    = static_cast<float>(dt.ConvertTo<CoreEngine::Units::Second>().Get());
            const float zoom_rate  = 5.0f;

            if (GetAsyncKeyState('E') & 0x8000)
            {
                speed += std::clamp(zoom_rate * speed, 1.0f, 200.0f) * dt_secs;
            }
            if (GetAsyncKeyState('Q') & 0x8000)
            {
                speed -= std::clamp(zoom_rate * speed, 1.0f, 200.0f) * dt_secs;
            }
            speed = std::clamp(speed, 0.1f, 1000.0f);
            s_free_cam_controller.SetMoveSpeed(speed);

            s_free_cam_controller.Update(m_free_cam_pseudo_camera, input_state, CoreEngine::Units::Convert<CoreEngine::Units::Second>(dt));
            out.SetPositionOpenGL_XYZ(m_free_cam_pseudo_camera.GetPosition());
            out.SetRotationOpenGL_WXYZ(m_free_cam_pseudo_camera.GetRotation());
            out.SetFovRadians(m_free_cam_pseudo_camera.GetFovRad());
        }
        else if (s_current_controller_type == CameraControllerType::ORBITAL_CAM)
        {
            try 
            {
                const CoreEngine::Units::Second dt_secs = CoreEngine::Units::Convert<CoreEngine::Units::Second>(dt);
                RacerState car_state (dll_state_out_opt_copy->m_racer_state);

                //Hack to get zoom working
                if (GetAsyncKeyState('E') & 0x8000)
                {
                    input_state.m_mouse_wheel_scroll_delta += 1.0f;
                }
                if (GetAsyncKeyState('Q') & 0x8000)
                {
                    input_state.m_mouse_wheel_scroll_delta -= 1.0f;
                }
                s_orbital_cam_controller.SetTarget(car_state.GetPositionOpenGL_XYZ());
                s_orbital_cam_controller.Update(m_orbital_cam_pseudo_camera, input_state, dt_secs);

                out.SetPositionOpenGL_XYZ(m_orbital_cam_pseudo_camera.GetPosition());
                out.SetRotationOpenGL_WXYZ(m_orbital_cam_pseudo_camera.GetRotation());
                out.SetFovRadians(m_orbital_cam_pseudo_camera.GetFovRad());
            }
            catch (const std::exception& e)
            {
                ENGINE_ERROR_PRINT("Exited ORBITAL Camera because of failure to obtain current Car State.");
                s_current_controller_type = CameraControllerType::FREE_CAM;
            }

        }
        else if (s_current_controller_type == CameraControllerType::FRONT_CAR)
        {
            try 
            {
                RacerState car_state (dll_state_out_opt_copy->m_racer_state);

                const CoreEngine::Units::Second dt_secs = CoreEngine::Units::Convert<CoreEngine::Units::Second>(dt);
                if (GetAsyncKeyState('E') & 0x8000)
                {
                    s_front_car_camera_controller.SetOffsetForward(s_front_car_camera_controller.GetOffsetForward() + 1.0f * dt_secs.Get());
                }
                if (GetAsyncKeyState('Q') & 0x8000)
                {
                    s_front_car_camera_controller.SetOffsetForward(s_front_car_camera_controller.GetOffsetForward() + -1.0f * dt_secs.Get());
                }
                s_front_car_camera_controller.SetRacerState(car_state);
                s_front_car_camera_controller.Update(m_front_car_cam_pseudo_camera, input_state, dt_secs);
                
                out.SetPositionOpenGL_XYZ(m_front_car_cam_pseudo_camera.GetPosition());
                out.SetRotationOpenGL_WXYZ(m_front_car_cam_pseudo_camera.GetRotation());
                out.SetFovRadians(m_front_car_cam_pseudo_camera.GetFovRad());
            } 
            catch (const std::exception& e)
            {
                ENGINE_ERROR_PRINT("Exited FRONT_CAR Camera because of failure to obtain current Car State.");
                s_current_controller_type = CameraControllerType::FREE_CAM;
            }
        }


        constexpr std::uint32_t flags = ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_FOV_RAD 
                                      | ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_POSITION 
                                      | ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_ROTATION;

        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd_ref_lock = AsphaltDllManager::GetDllGeneralCommandsInRef(); 
        general_cmd_ref_lock->m_write_camera_state = out.ToWriteCameraState();
        general_cmd_ref_lock->m_write_camera_state.m_continuous_override_on_flags = flags;
        general_cmd_ref_lock->m_write_camera_state.m_command_type = ComDllIn::CommandType::ExecuteCommand;
    }

    void CameraToolLayer::OnRender() noexcept 
    {

    }

    void CameraToolLayer::OnImGuiRender() noexcept 
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

        if (! ImGui::Begin("Camera Tool", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        { // Scope to delete scoped styles
            ImGui::CollapsingHeader("Camera Tool", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf);

            std::optional<ComDllOut::DllStateOut> dll_state_out_opt_copy = AsphaltDllManager::GetDllStateOutCopy();
            ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd_ref_lock = AsphaltDllManager::GetDllGeneralCommandsInRef(); 

            int controller_type = static_cast<int>(s_current_controller_type);
            if (ImGui::RadioButton("Free Flight", &controller_type, static_cast<int>(CameraControllerType::FREE_CAM)))
            {
                MouseInputService::LaunchThread();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Orbital", &controller_type, static_cast<int>(CameraControllerType::ORBITAL_CAM)))
            {
                MouseInputService::LaunchThread();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Car Front", &controller_type, static_cast<int>(CameraControllerType::FRONT_CAR)))
            {
                MouseInputService::StopThread();
            }
            s_current_controller_type = static_cast<CameraControllerType>(controller_type);

            if (s_current_controller_type == CameraControllerType::FREE_CAM)
            {
                float velo = s_free_cam_controller.GetMoveSpeed();
                if (ImGui::SliderFloat("Fly Speed", &velo, 0.1, 1000.0f, "%.3f"))
                {
                    s_free_cam_controller.SetMoveSpeed(velo);
                }

                float sensitivity = s_free_cam_controller.GetSensitivity();
                if (ImGui::SliderFloat("Sensitivity", &sensitivity, 0.01f, 0.5f, "%.3f"))
                {
                    s_free_cam_controller.SetSensitivity(sensitivity);
                }

                float fov_deg = m_free_cam_pseudo_camera.GetFovDeg();
                if (ImGui::SliderFloat("Fov", &fov_deg, 10.0f, 160.0f, "%.1f°"))
                {
                    m_free_cam_pseudo_camera.SetFovDeg(fov_deg);
                }

                if (ImGui::SliderInt("Game Speed", (int*)&general_cmd_ref_lock->m_write_meta_data.m_fixed_frame_interval_micros, 0, 25'000))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                bool enable_mouse_input = MouseInputService::GetThreadIsRunning();
                if (ImGui::Checkbox("Enable Mouse Input", &enable_mouse_input))
                {
                    if (enable_mouse_input)
                        MouseInputService::LaunchThread();
                    else 
                        MouseInputService::StopThread();
                }

                if (enable_mouse_input)
                {
                    ImGui::SameLine();
                    bool recenter = MouseInputService::GetAlwaysRecenterCursor();
                    if (ImGui::Checkbox("Lock Mouse Position", &recenter))
                    {
                        MouseInputService::SetAlwaysRecenterCursor(recenter);
                    }
                }

                if (ImGui::Checkbox("Hide Gui", &general_cmd_ref_lock->m_write_meta_data.m_hide_gui))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                if (ImGui::Button("Move To"))
                {
                    m_free_cam_pseudo_camera.SetPosition(m_gui_free_cam_input_position);
                }
                ImGui::SameLine();
                ImGui::InputFloat3("##input_tp", glm::value_ptr(m_gui_free_cam_input_position));

                if (ImGui::Button("Move to Car"))
                {
                    try 
                    {
                        RacerState car_state (dll_state_out_opt_copy->m_racer_state);
                        m_free_cam_pseudo_camera.SetPosition(car_state.GetPositionOpenGL_XYZ());
                    }
                    catch (const std::exception& e)
                    {
                        ENGINE_ERROR_PRINT("Failed to move to car because of failure to obtain racer state");
                    }
                }

                ImGui::TextUnformatted(("Position : " + CoreEngine::CommonUtility::GlmVec3ToString(m_free_cam_pseudo_camera.GetPosition())).c_str());
                ImGui::TextUnformatted(("Rotation : " + CoreEngine::CommonUtility::GlmQuatToString(m_free_cam_pseudo_camera.GetRotation())).c_str());
                ImGui::TextUnformatted(("Rot Euler: " + CoreEngine::CommonUtility::GlmVec3ToString(glm::degrees(glm::eulerAngles(m_free_cam_pseudo_camera.GetRotation())))).c_str());
            }
            else if (s_current_controller_type == CameraControllerType::ORBITAL_CAM)
            {
                float sensitivity = s_orbital_cam_controller.GetSensitivity();
                if (ImGui::SliderFloat("Sensitivity", &sensitivity, 0.01f, 0.5f, "%.3f"))
                {
                    s_orbital_cam_controller.SetSensitivity(sensitivity);
                }

                float fov_deg = m_orbital_cam_pseudo_camera.GetFovDeg();
                if (ImGui::SliderFloat("Fov", &fov_deg, 10.0f, 160.0f, "%.1f°"))
                {
                    m_orbital_cam_pseudo_camera.SetFovDeg(fov_deg);
                }

                float distance = s_orbital_cam_controller.GetDistance();
                if (ImGui::SliderFloat("Distance", &distance, 0.1f, 20.0f, "%.3f"))
                {
                    s_orbital_cam_controller.SetDistance(distance);
                }

                bool enable_mouse_input = MouseInputService::GetThreadIsRunning();
                if (ImGui::Checkbox("Enable Mouse Input", &enable_mouse_input))
                {
                    if (enable_mouse_input)
                        MouseInputService::LaunchThread();
                    else 
                        MouseInputService::StopThread();
                }

                if (ImGui::SliderInt("Game Speed", (int*)&general_cmd_ref_lock->m_write_meta_data.m_fixed_frame_interval_micros, 0, 25'000))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                if (enable_mouse_input)
                {
                    ImGui::SameLine();
                    bool recenter = MouseInputService::GetAlwaysRecenterCursor();
                    if (ImGui::Checkbox("Lock Mouse Position", &recenter))
                    {
                        MouseInputService::SetAlwaysRecenterCursor(recenter);
                    }
                }

                if (ImGui::Checkbox("Hide Gui", &general_cmd_ref_lock->m_write_meta_data.m_hide_gui))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                ImGui::TextUnformatted(("Position : " + CoreEngine::CommonUtility::GlmVec3ToString(m_orbital_cam_pseudo_camera.GetPosition())).c_str());
                ImGui::TextUnformatted(("Rotation : " + CoreEngine::CommonUtility::GlmQuatToString(m_orbital_cam_pseudo_camera.GetRotation())).c_str());
                ImGui::TextUnformatted(("Rot Euler: " + CoreEngine::CommonUtility::GlmVec3ToString(glm::degrees(glm::eulerAngles(m_orbital_cam_pseudo_camera.GetRotation())))).c_str());
            }
            else if (s_current_controller_type == CameraControllerType::FRONT_CAR)
            {
                float fov_deg = m_front_car_cam_pseudo_camera.GetFovDeg();
                if (ImGui::SliderFloat("Fov", &fov_deg, 10.0f, 160.0f, "%.1f°"))
                {
                    m_front_car_cam_pseudo_camera.SetFovDeg(fov_deg);
                }

                float offset_right = s_front_car_camera_controller.GetOffsetRight();
                if (ImGui::SliderFloat("Offset Right", &offset_right, -5.0f, 5.0f))
                {
                    s_front_car_camera_controller.SetOffsetRight(offset_right);
                }
                
                float offset_up = s_front_car_camera_controller.GetOffsetUp();
                if (ImGui::SliderFloat("Offset Up", &offset_up, -5.0f, 5.0f))
                {
                    s_front_car_camera_controller.SetOffsetUp(offset_up);
                }

                float offset_forward = s_front_car_camera_controller.GetOffsetForward();
                if (ImGui::SliderFloat("Offset Forward", &offset_forward, -10.0f, 10.0f))
                {
                    s_front_car_camera_controller.SetOffsetForward(offset_forward);
                }

                if (ImGui::SliderInt("Game Speed", (int*)&general_cmd_ref_lock->m_write_meta_data.m_fixed_frame_interval_micros, 0, 25'000))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                bool look_backwards = s_front_car_camera_controller.GetLookBackwards();
                if (ImGui::Checkbox("Look Backwards", &look_backwards))
                {
                    s_front_car_camera_controller.SetLookBackwards(look_backwards);
                }

                if (ImGui::Checkbox("Hide Gui", &general_cmd_ref_lock->m_write_meta_data.m_hide_gui))
                {
                    general_cmd_ref_lock->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
                }

                ImGui::TextUnformatted(("Position : " + CoreEngine::CommonUtility::GlmVec3ToString(m_orbital_cam_pseudo_camera.GetPosition())).c_str());
                ImGui::TextUnformatted(("Rotation : " + CoreEngine::CommonUtility::GlmQuatToString(m_orbital_cam_pseudo_camera.GetRotation())).c_str());
                ImGui::TextUnformatted(("Rot Euler: " + CoreEngine::CommonUtility::GlmVec3ToString(glm::degrees(glm::eulerAngles(m_orbital_cam_pseudo_camera.GetRotation())))).c_str());
            }
            else 
            {
                ENGINE_ASSERT(false && "Unkown camera controller: Should not be reachable.");
            }
        }

        ImGui::End();
    }

    void CameraToolLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one FreeFlightLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        const CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "Camera Tool ",
            .m_relative_size               = {500.0f / 1920.0f, 300.0f / 1080.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = false,
            .m_is_clickthrough             = false
        };
        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<CameraToolLayer>(config);
    }

    bool CameraToolLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void CameraToolLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }
}
