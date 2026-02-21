#pragma once

//Own includes
#include "core/event/Event.h"
#include "core/utility/Units.h"
#include "core/application/Window.h"

//std
#include <cstdint>

namespace CoreEngine
{
    class Basic_Layer
    {
    protected:
        Window::Handle m_handle;

    public:
        explicit Basic_Layer(Window::Handle window_handle) noexcept : m_handle(window_handle) {}
        virtual ~Basic_Layer() noexcept = default;

        virtual void OnUpdate(Units::MicroSecond delta_time) noexcept    = 0;
        virtual void OnEvent(Basic_Event& event) noexcept                = 0;
        virtual void OnRender() noexcept                                 = 0;
        virtual void OnImGuiRender() noexcept                            = 0;
    };
}