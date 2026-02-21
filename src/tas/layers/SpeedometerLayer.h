#pragma once

#include "core/layer/Layer.h"

namespace AsphaltTas
{
    class SpeedometerLayer : public CoreEngine::Basic_Layer
    {
    public:
        explicit SpeedometerLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~SpeedometerLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;

    private:
        void OnLock() noexcept;
        void OnUnlock() noexcept;
        static inline SpeedometerLayer* s_instance = nullptr;

        float m_font_size = 5.0f;
        bool m_is_locked  = false;
        bool m_left_mouse_pressed_after_unlock_disable_gui_input = false;
    };
}