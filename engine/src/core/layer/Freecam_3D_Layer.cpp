#include "core/layer/Freecam_3D_Layer.h"

//Own includes
#include "core/application/ApplicationGlobalState.h"

#include "core/utility/Assert.h"

#include "core/scene/FreeCam_CameraController.h"

//GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//ImGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuiFileDialog.h>

//std
#include <array>

namespace CoreEngine
{
    //-------- Public methods
    Freecam_3D_Layer::Freecam_3D_Layer() noexcept 
    : m_camera(glm::vec3(0.0f), GlobalGet<GlobalGet_AspectRatio>(), 50, 0.1f), m_camera_controller(std::make_unique<FreeCam_CameraController>())
    {
        m_scene.SetDebugDrawer(&m_bt_debug_draw_pipeline);
    }

    //--------- Implementation of Basic_Layers abstract methods 
    void Freecam_3D_Layer::OnUpdate(Units::MicroSecond delta_time)
    {
        //---- Physics updates
        if (m_update_scene_physics) 
        {
            m_scene.OnPhysicsUpdate(delta_time * m_physics_speed_scale);
        }
        //---- Input
        m_camera_controller->Update(m_camera, m_input_state, Units::Convert<Units::Second>(delta_time));
        m_input_state.m_mouse_move_delta         = {0, 0};
        m_input_state.m_mouse_wheel_scroll_delta = 0.0;
        
        //---- Frame camera matrix
        const glm::mat4 cam_matrix = m_camera.CalculateCameraMatrix();

        //---- If an object was added / deleted, Update the entire data - else just update transformations
        if (m_scene.GetAndResetObjectVecChangeFlag())  { m_pipeline.SetSceneData(m_scene.GetRenderModelVector(), m_scene.GetLightVectorConstRef()); } 
        else                                           { m_pipeline.UpdateModelTransforms(m_scene.GetRenderModelVector(), cam_matrix); }

        if (m_scene.GetAndResetLightVecChangeFlag())   { m_pipeline.SetLightData(m_scene.GetLightVectorConstRef()); } 

        m_pipeline.SetCameraData(cam_matrix, m_camera.GetPosition());
        m_bt_debug_draw_pipeline.SetCameraMatrixAndFrustumCull(cam_matrix);
    }

    void Freecam_3D_Layer::OnEvent(Basic_Event& event) 
    {   
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(         [this](KeyPressedEvent&  e)         -> bool { return OnKeyPressed(e);          });
        dispatcher.Dispatch<KeyReleasedEvent>(        [this](KeyReleasedEvent& e)         -> bool { return OnKeyReleased(e);         });
        dispatcher.Dispatch<MousePressedEvent>(       [this](MousePressedEvent& e)        -> bool { return OnMousePressed(e);        });
        dispatcher.Dispatch<MouseReleasedEvent>(      [this](MouseReleasedEvent& e)       -> bool { return OnMouseReleased(e);       });
        dispatcher.Dispatch<MouseMovedEvent>(         [this](MouseMovedEvent& e)          -> bool { return OnMouseMoved(e);          });
        dispatcher.Dispatch<MouseScrolledEvent>(      [this](MouseScrolledEvent& e)       -> bool { return OnMouseScrolled(e);       });
        dispatcher.Dispatch<FramebufferResizeEvent>(  [this](FramebufferResizeEvent& e)   -> bool { return OnFramebufferResize(e);   });
        dispatcher.Dispatch<ApplicationShutdownEvent>([this](ApplicationShutdownEvent& e) -> bool { return OnApplicationShutdown(e); });
    }

    void Freecam_3D_Layer::OnRender()
    {
        m_pipeline.Render();

        if (m_draw_bullet_debug)
        {
            m_scene.OnDrawBtDebug();
            m_bt_debug_draw_pipeline.RenderAndClearData();
        }
    }

    void Freecam_3D_Layer::OnImGuiRender() 
    {
        const std::string unique_window_name = std::format("Freecam3D_Layer##{}", (void*)this);
        ImGui::Begin(unique_window_name.c_str());

        ImGui::Checkbox("Update Physics ", &m_update_scene_physics);

        bool vsync_now = GlobalGet<GlobalGet_VsyncIsOn>();
        ImGui::Checkbox("Toggle Vsync", &vsync_now);
        GlobalSet<GlobalSet_VsyncIsOn>(vsync_now);

        ImGui::SliderFloat(":Speed-factor ", &m_physics_speed_scale, 0.01f, 20.0f);

        float fov = m_camera.GetFovDeg();
        ImGui::SliderFloat(":Fov ", &fov, 20.0f, 120.0f);
        m_camera.SetFovDeg(fov);
        
        ImGui::Text( ("-----Camera----\n" + m_camera.ToString() + "\n---------------").c_str());

        ImGui::End();   
    }

    bool Freecam_3D_Layer::OnKeyPressed(KeyPressedEvent& e) noexcept
    { 
        if (ImGui::GetIO().WantCaptureKeyboard) return false;

        m_input_state.m_key_is_pressed[e.GetKeyType()] = true; 
        return false; 
    }

    bool Freecam_3D_Layer::OnKeyReleased(KeyReleasedEvent& e) noexcept
    {
        m_input_state.m_key_is_pressed[e.GetKeyType()] = false; 
        return false; 
    }

    bool Freecam_3D_Layer::OnMousePressed(MousePressedEvent& e) noexcept
    {
        if (ImGui::GetIO().WantCaptureMouse) return false;
        
        m_input_state.m_mouse_is_pressed[e.GetMouseButton()] = true;
        if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_RIGHT) m_input_state.m_previous_mouse_pos.reset(); //To prevent flicks when clicking right mouse again
        return false;
    }

    bool Freecam_3D_Layer::OnMouseReleased(MouseReleasedEvent& e) noexcept
    {
        m_input_state.m_mouse_is_pressed[e.GetMouseButton()] = false;
        if (e.GetMouseButton() == GLFW_MOUSE_BUTTON_RIGHT) m_input_state.m_previous_mouse_pos.reset();
        return false;
    }

    bool Freecam_3D_Layer::OnMouseMoved(MouseMovedEvent& e) noexcept
    {
        if (ImGui::GetIO().WantCaptureMouse) //Only turn camera while right mouse is pressed
            return false;

        if (m_input_state.m_previous_mouse_pos.has_value())
        {
            const glm::ivec2 prev = *m_input_state.m_previous_mouse_pos;
            m_input_state.m_mouse_move_delta.x += e.GetX() - prev.x;
            m_input_state.m_mouse_move_delta.y += e.GetY() - prev.y;
        }

        m_input_state.m_previous_mouse_pos = { e.GetX(), e.GetY() };

        return false;
    }

    bool Freecam_3D_Layer::OnMouseScrolled(MouseScrolledEvent& e) noexcept
    {
        m_input_state.m_mouse_wheel_scroll_delta += e.GetYOffset();
        return false;
    }

    bool Freecam_3D_Layer::OnFramebufferResize(FramebufferResizeEvent& e) noexcept
    {
        m_camera.SetAspectRatio(static_cast<float>(e.GetWidth()) / e.GetHeight());
        return false;
    }

    bool Freecam_3D_Layer::OnApplicationShutdown(ApplicationShutdownEvent& e) noexcept
    {
        return false;
    }
}