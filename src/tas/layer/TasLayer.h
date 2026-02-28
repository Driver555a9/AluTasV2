#pragma once

#include "core/layer/Layer.h"
#include "core/utility/Units.h"

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
    };
}