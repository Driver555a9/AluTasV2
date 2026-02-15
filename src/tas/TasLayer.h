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

    private:
        void OnImGuiRender_FreeFlightOptions() noexcept;
        bool OnAttemptUpdateAddresses(bool force_update_each) noexcept;
        bool IsInFreeFlight() const noexcept;

        CoreEngine::CameraReverseZ           m_pseudo_game_camera;
        CoreEngine::InputState               m_pseudo_game_input_state;
        CoreEngine::FreeCam_CameraController m_pseudo_game_camera_controller;

        bool m_free_flight_recenter = true;

        //////////////////////////////////////////////////////////
        // GUI design
        //////////////////////////////////////////////////////////
        static constexpr const ImVec4 COLOR_RED          {0.8f, 0, 0, 1.0f};
        static constexpr const ImVec4 COLOR_GREEN        {0, 0.8f, 0, 1};
        static constexpr const ImVec4 COLOR_BLUE         {0, 0, 0.8f, 1};
        static constexpr const ImVec4 COLOR_ORANGE       {1.0f, 0.518f, 0.0f, 1.0f};
        static constexpr const ImVec4 COLOR_TRANSPARENT  {0, 0, 0, 0};

        static constexpr const ImVec4 COLOR_BACKGROUND   {0.1f, 0.1f, 0.1f, 1.0f};
        static constexpr const ImVec4 COLOR_BORDER       {0.5f, 0.5f, 0.5f, 0.8f};
        static constexpr const ImVec4 COLOR_TEXT         {1, 1, 1, 1};

        static constexpr const ImVec4 COLOR_X_AXYS       { COLOR_RED   };
        static constexpr const ImVec4 COLOR_Y_AXYS       {0.f, 0.796f, 1.f, 1.0f};
        static constexpr const ImVec4 COLOR_Z_AXYS       { COLOR_GREEN };
    };
}