#pragma once

#include "core/layer/Layer.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/scene/OrbitalCam_CameraController.h"
#include "core/scene/FollowCam_CameraController.h"
#include "core/scene/DummyCameraController.h"
#include "core/utility/Timer.h"

#include "tas/common/FrontCar_CameraController.h"

#include "glm/glm.hpp"

namespace AsphaltTas
{
    class CameraToolLayer : public CoreEngine::Basic_Layer 
    {
    public:
        explicit CameraToolLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~CameraToolLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;

    private:
        static inline CameraToolLayer* s_instance = nullptr;

        CoreEngine::CameraReverseZ m_free_cam_pseudo_camera;
        static inline CoreEngine::FreeCam_CameraController s_free_cam_controller = CoreEngine::FreeCam_CameraController();
        
        CoreEngine::CameraReverseZ m_orbital_cam_pseudo_camera;
        static inline CoreEngine::OrbitalCam_CameraController s_orbital_cam_controller = CoreEngine::OrbitalCam_CameraController();

        CoreEngine::CameraReverseZ m_front_car_cam_pseudo_camera;
        static inline FrontCar_CameraControler s_front_car_camera_controller = FrontCar_CameraControler();

        enum class CameraControllerType
        {
            FREE_CAM, ORBITAL_CAM, FRONT_CAR
        };

        static inline CameraControllerType s_current_controller_type = CameraControllerType::FREE_CAM;

        //Gui options relative to the specific tpype of camera controller
        glm::vec3 m_gui_free_cam_input_position {0};
    };
}