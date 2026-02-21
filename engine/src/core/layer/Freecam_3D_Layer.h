#pragma once

//Own includes
#include "core/layer/Layer.h"

#include "core/rendering/IndirectDraw3D_RenderPipeline.h"
#include "core/rendering/BulletDebugDraw_RenderPipeline.h"

#include "core/scene/Scene3D.h"
#include "core/scene/Camera.h"
#include "core/scene/CameraController.h"

#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"
#include "core/event/InputEvents.h"
#include "core/event/ApplicationStateEvents.h"

namespace CoreEngine
{
    class Freecam_3D_Layer : public Basic_Layer
    {
    public:
        explicit Freecam_3D_Layer(Window::Handle window_handle) noexcept;
        virtual ~Freecam_3D_Layer() noexcept = default;

    //------- Allowed default move
        Freecam_3D_Layer(Freecam_3D_Layer&&) noexcept            = default;
        Freecam_3D_Layer& operator=(Freecam_3D_Layer&&) noexcept = default;

    //------- Forbidden copy from Scene3D and IndirectDraw3D_RenderingPipeline
        Freecam_3D_Layer(const Freecam_3D_Layer&)            = delete;
        Freecam_3D_Layer& operator=(const Freecam_3D_Layer&) = delete;

    //------- Implementations of Basic_Layers abstract methods
        virtual void OnUpdate(Units::MicroSecond delta_time) noexcept override;
        virtual void OnEvent(Basic_Event& event) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;
    //-------
    
    protected:
        CameraReverseZ m_camera;
        Scene3D m_scene;

        IndirectDraw3D_RenderPipeline   m_pipeline{};
        BulletDebugDraw_RenderPipeline  m_bt_debug_draw_pipeline{};

        std::unique_ptr<Basic_CameraController> m_camera_controller = nullptr;
        InputState m_input_state;

    //------- Physics
        float m_physics_speed_scale  {1.0f};
        bool  m_update_scene_physics {false};
        bool  m_draw_bullet_debug    {false};

    //------- Protected methods
        [[nodiscard]] virtual bool OnKeyPressed(KeyPressedEvent& e) noexcept;
        [[nodiscard]] virtual bool OnKeyReleased(KeyReleasedEvent& e) noexcept;
        [[nodiscard]] virtual bool OnMousePressed(MousePressedEvent& e) noexcept;
        [[nodiscard]] virtual bool OnMouseReleased(MouseReleasedEvent& e) noexcept;
        [[nodiscard]] virtual bool OnMouseMoved(MouseMovedEvent& e) noexcept;
        [[nodiscard]] virtual bool OnMouseScrolled(MouseScrolledEvent& e) noexcept;
        [[nodiscard]] virtual bool OnFramebufferResize(FramebufferResizeEvent& e) noexcept;
        [[nodiscard]] virtual bool OnApplicationShutdown(ApplicationShutdownEvent& e) noexcept;
    };
}