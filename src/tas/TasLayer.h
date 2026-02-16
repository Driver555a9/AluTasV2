#pragma once

#include "core/layer/Layer.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/utility/Timer.h"

#include "imgui/imgui.h"

namespace AsphaltTas
{
    class TasLayer : public CoreEngine::Basic_Layer
    {
    public:
        explicit TasLayer() noexcept;
        virtual ~TasLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) override;
        virtual void OnRender() override;
        virtual void OnImGuiRender() override;

    private:;
        bool OnAttemptUpdateAddresses(bool force_update_each) noexcept;
        bool IsInFreeFlight() const noexcept;

        CoreEngine::CameraReverseZ           m_pseudo_game_camera;
        CoreEngine::InputState               m_pseudo_game_input_state;
        CoreEngine::FreeCam_CameraController m_pseudo_game_camera_controller;

        glm::vec3 m_free_cam_input_position {0};

        //////////////////////////////////////////////////////////
        // GUI design
        //////////////////////////////////////////////////////////

        static constexpr const ImVec4 COLOR_RED          {0.8f, 0, 0, 1.0f};
        static constexpr const ImVec4 COLOR_GREEN        {0, 0.8f, 0, 1};
        static constexpr const ImVec4 COLOR_BLUE         {0, 0, 0.8f, 1};
        static constexpr const ImVec4 COLOR_ORANGE       {1.0f, 0.518f, 0.0f, 1.0f};
        static constexpr const ImVec4 COLOR_TRANSPARENT  {0, 0, 0, 0};
        static constexpr const ImVec4 COLOR_BLACK        {0, 0, 0, 1};
    };
}