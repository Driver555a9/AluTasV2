#pragma once

#include "core/layer/Layer.h"
#include "core/scene/Camera.h"
#include "core/model/BoxModel.h"
#include "core/utility/Assert.h"
#include "tas/common/Replay.h"

#include "tas/rendering/DrawGhost_RenderPipeline.h"

#include <optional>

namespace AsphaltTas
{
    class GhostRenderLayer : public CoreEngine::Basic_Layer 
    {
    public:
        explicit GhostRenderLayer(CoreEngine::Window::Handle handle) noexcept;
        virtual ~GhostRenderLayer() noexcept;    

        virtual void OnEvent(CoreEngine::Basic_Event& e) noexcept override;
        virtual void OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept override;
        virtual void OnRender() noexcept override;
        virtual void OnImGuiRender() noexcept override;

        static void CreateInstance() noexcept;
        [[nodiscard]] static bool InstanceExists() noexcept;
        static void DeleteInstance() noexcept;

        [[nodiscard]] static glm::vec4 GetGhostColor() noexcept;
        static void SetGhostColor(const glm::vec4& color) noexcept;

        [[nodiscard]] static bool GetIsUsingCustomColorShader() noexcept;
        static void SetUseCustomColorShader(bool on) noexcept;

        static void SetCurrentReplay(const std::optional<Replay>& replay) noexcept;
        [[nodiscard]] static const std::optional<Replay>& GetCurrentReplayConstRef() noexcept;
        
        [[nodiscard]] static bool GetReplayPlaybackIsOn() noexcept;
        static void SetEnableReplayPlayback(bool enable) noexcept;

        [[nodiscard]] static std::unique_ptr<CoreEngine::Basic_Model>& GetCurrentGhostModel() noexcept;

        template <typename TModel, typename... Args>
        requires std::is_base_of_v<CoreEngine::Basic_Model, TModel> && std::is_constructible_v<TModel, Args...>
        static inline bool CreateCustomGhostModel(Args&&... args) noexcept
        {
            try 
            {
                s_ghost_model = std::make_unique<TModel>(std::forward<Args>(args)...);
            } 
            catch (const std::exception& e)
            {
                ENGINE_ERROR_PRINT("Failed to load Ghost Model: " << e.what());
                return false;
            }
            return true;
        }
        
    private: 
        static inline GhostRenderLayer* s_instance = nullptr;

        static inline std::unique_ptr<CoreEngine::Basic_Model> s_ghost_model = std::make_unique<CoreEngine::BoxModel>(glm::vec3(1.0f), glm::vec3{}, glm::quat{}, glm::vec3(1.0f));
        static inline std::optional<Replay> s_replay_to_race_against = std::nullopt;
        static inline bool s_race_against_replay = false;

        DrawGhost_RenderPipeline m_render_pipeline{};
        CoreEngine::CameraReverseZ m_camera;

        /////////////////////////////////////////
        // Gui state
        /////////////////////////////////////////
        static inline glm::vec4 s_ghost_color {0.0f, 0.0f, 1.0f, 0.8f };
        static inline bool s_use_custom_color_shader = true;
    };
}