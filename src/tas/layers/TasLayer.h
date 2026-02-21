#pragma once

#include "core/layer/Layer.h"
#include "core/scene/FreeCam_CameraController.h"
#include "core/utility/Timer.h"

#include "core/scene/Scene3D.h"
#include "core/rendering/IndirectDraw3D_RenderPipeline.h"

#include "imgui/imgui.h"

namespace AsphaltTas
{
    class TasLayer : public CoreEngine::Basic_Layer
    {
    public:
        explicit TasLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~TasLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

    private:
        void OnRenderGhostExperimental() noexcept;
    };
}