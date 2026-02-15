#pragma once

//Own includes
#include "core/event/Event.h"

#include "core/utility/Units.h"

//std
#include <cstdint>

namespace CoreEngine
{
    class Basic_Layer
    {
        public:
            virtual ~Basic_Layer() noexcept = default;

            virtual void OnUpdate(Units::MicroSecond delta_time)     = 0;
            virtual void OnEvent(Basic_Event& event)                 = 0;
            virtual void OnRender()                                  = 0;
            virtual void OnImGuiRender()                             = 0;
    };
}