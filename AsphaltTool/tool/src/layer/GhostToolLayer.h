#pragma once

#include "core/layer/Layer.h"

namespace AsphaltTas
{
    class GhostToolLayer : public CoreEngine::Basic_Layer 
    {
    public:
        explicit GhostToolLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~GhostToolLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;
        
    private: 
        static inline GhostToolLayer* s_instance = nullptr;

        /////////////////////////////////
        // Gui state
        /////////////////////////////////
        std::string m_current_loaded_model;
    };
}