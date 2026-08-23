#include "core/layer/Layer.h"

#include "core/rendering/BulletDebugDraw_RenderPipeline.h"
#include "core/scene/Camera.h"

namespace AsphaltTas
{
    class DebugDrawStreamLayer : public CoreEngine::Basic_Layer
    {
    public:
        explicit DebugDrawStreamLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~DebugDrawStreamLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override; 

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;
        
    private:
        static inline DebugDrawStreamLayer* s_instance = nullptr;
        CoreEngine::BulletDebugDraw_RenderPipeline m_bullet_debug_pipeline;
        CoreEngine::CameraReverseZ m_camera;
        bool m_has_signal = false;
    };

};