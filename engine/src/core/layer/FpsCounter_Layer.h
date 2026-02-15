#pragma once

//Own includes
#include "core/layer/Layer.h"

namespace CoreEngine
{
    class FpsCounter_Layer : public Basic_Layer 
    {
        public:

            FpsCounter_Layer() = default;
            virtual ~FpsCounter_Layer() = default;

        //------- Implementations of Basic_Layers abstract methods
            virtual void OnUpdate(Units::MicroSecond delta_time) override {}
            virtual void OnEvent(Basic_Event& event) override {}
            virtual void OnRender() override {}
            virtual void OnImGuiRender() override;
        //-------
    };
}