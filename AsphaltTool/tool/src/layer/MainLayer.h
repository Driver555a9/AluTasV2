#pragma once

#include "core/layer/Layer.h"
#include "core/utility/Units.h"

namespace AsphaltTas
{
    class MainLayer : public CoreEngine::Basic_Layer
    {
    public:
        explicit MainLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~MainLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;

    private:
        static inline MainLayer* s_instance = nullptr;
    };
}