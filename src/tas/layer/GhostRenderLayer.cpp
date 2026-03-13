#include "tas/layer/GhostRenderLayer.h"

#include "core/utility/Assert.h"
#include "core/application/Application.h"
#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"
#include "core/model/BoxModel.h"

#include "tas/common/CameraState.h"
#include "tas/common/RacerState.h"
#include "tas/common/RaceProgressState.h"
#include "tas/globalstate/GameState.h"
#include "tas/memory/MemoryUtility.h"
#include "tas/servicethreads/ReadCurrentStateService.h"
#include "tas/servicethreads/ReplayRecorderService.h"

#ifdef _WIN32
    #include <windows.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#endif


namespace AsphaltTas
{

namespace 
{
#ifdef _WIN32
    WNDPROC g_original_proc = nullptr;
    LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_NCHITTEST:
                return HTTRANSPARENT;

            case WM_MOUSEACTIVATE:
                return MA_NOACTIVATE;

            case WM_SETFOCUS:
                return 0;

            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                return 0;
        }

        return CallWindowProc(g_original_proc, hwnd, uMsg, wParam, lParam);
    }
#endif 
}

    GhostRenderLayer::GhostRenderLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle),
    m_camera (glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 55.0f, 0.5f)
    {
        ENGINE_ASSERT(! InstanceExists() && "There must not be two Ghost Render Layer at the same time.");
        s_instance = this;

        m_render_pipeline.SetGhostData(s_ghost_model.get());
        SetUseCustomColorShader(s_use_custom_color_shader);
        m_render_pipeline.SetGhostColor(s_ghost_color);

    #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetGLFWwindow());
        g_original_proc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)OverlayWndProc);
    #endif
    }

    GhostRenderLayer::~GhostRenderLayer() noexcept
    {
        s_instance = nullptr;
    }

    void GhostRenderLayer::OnEvent(CoreEngine::Basic_Event& e) noexcept
    {
        CoreEngine::EventDispatcher disp(e);
        disp.Dispatch<CoreEngine::FramebufferResizeEvent>([this](CoreEngine::FramebufferResizeEvent& e)
        { 
            m_camera.SetAspectRatio(e.GetAspectRatio());
            return false; 
        });
    }

    void GhostRenderLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
    {

    }

    void GhostRenderLayer::OnRender() noexcept
    {
        ENGINE_ASSERT(s_ghost_model && "There must be a ghost model before GhostRenderLayer::OnUpdate() may be called.");

    #ifdef _WIN32
        HWND asphalt_hwnd = GameState::GetGameHWNDOrNullptr();
        if (asphalt_hwnd)
        {
            MemoryUtility::WindowInfo info;
            try 
            {
                info = MemoryUtility::GetWindowInfoOrThrow(asphalt_hwnd);
                GLFWwindow* window = CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetGLFWwindow();
                glfwSetWindowPos(window, info.m_x_position, info.m_y_position);
                glfwSetWindowSize(window, info.m_width, info.m_height);
            }
            catch (const std::exception& e) { ENGINE_ERROR_PRINT(e.what()); }
        }
    #endif

        // If playback is off, or no replay is currently loaded, render ghost at car position isntead
        if (! s_race_against_replay || ! s_replay_to_race_against.has_value() || s_replay_to_race_against->GetAmountFrames() == 0)
        {
            CameraState camera_state_now;
            RacerState racer_state;
            try 
            {
                camera_state_now = ReadCurrentStateService::GetCurrentCameraState().value();
                racer_state      = ReadCurrentStateService::GetCurrentRacerState().value();

                s_ghost_model->SetPosition(racer_state.GetExtractedPosition());
                s_ghost_model->SetRotation(racer_state.GetExtractedRotation());
                m_camera.SetPosition(camera_state_now.m_position);
                m_camera.SetRotation(camera_state_now.m_rotation);
                m_camera.SetFovRad(camera_state_now.m_fov_radians);

                m_render_pipeline.SetGhostData(s_ghost_model.get());
                m_render_pipeline.SetCameraData(m_camera.CalculateCameraMatrix(), m_camera.GetPosition());
                m_render_pipeline.Render();
            } 
            catch (...) 
            {  
                m_render_pipeline.Render(); 
            }

            return;
        }
        
        //If replay, render ghost at replay pos
        try 
        {
            CameraState camera_state_now    = ReadCurrentStateService::GetCurrentCameraState().value();
            RaceProgressState race_progress = ReadCurrentStateService::GetCurrentRaceProgressState().value();

            s_replay_to_race_against->GoToFirstFrameOverGivenLapTime(race_progress.m_lap_time);
            const AsphaltTas::Replay::Frame frame = s_replay_to_race_against->GetCurrentFrame().value();
            
            s_ghost_model->SetPosition(frame.m_racer_state.GetExtractedPosition());
            s_ghost_model->SetRotation(frame.m_racer_state.GetExtractedRotation());
            m_camera.SetPosition(camera_state_now.m_position);
            m_camera.SetRotation(camera_state_now.m_rotation);
            m_camera.SetFovRad(camera_state_now.m_fov_radians);

            m_render_pipeline.SetGhostData(s_ghost_model.get());
            if (s_replay_ghost_racing_line.has_value())
            {
                m_render_pipeline.SetRacingLineData(s_replay_ghost_racing_line.value());
            } else { ENGINE_ERROR_PRINT("[Debug] Failed to push racing line data to line rendering pipeline, because optional is empty."); }
            
            m_render_pipeline.SetCameraData(m_camera.CalculateCameraMatrix(), m_camera.GetPosition());
            m_render_pipeline.Render();

        } 
        catch (...) 
        {
            m_render_pipeline.Render();
        }
    }

    void GhostRenderLayer::OnImGuiRender() noexcept
    {

    }

    void GhostRenderLayer::CreateInstance() noexcept
    {
        if (InstanceExists()) return;

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "Ghost Render",
            .m_relative_size               = {1.0f, 1.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 8,
            .m_is_decorated                = false,
            .m_has_transparent_framebuffer = true,
            .m_is_clickthrough             = true
        };

        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<GhostRenderLayer>(config);
    }

    bool GhostRenderLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void GhostRenderLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }
    
    glm::vec4 GhostRenderLayer::GetGhostColor() noexcept
    {
        return s_ghost_color;
    }

    void GhostRenderLayer::SetGhostColor(const glm::vec4& color) noexcept
    {
        s_ghost_color = color;
        if (InstanceExists())
        {
            s_instance->m_render_pipeline.SetGhostColor(color);
        }
    }

    bool GhostRenderLayer::GetIsUsingCustomColorShader() noexcept
    {
        return s_use_custom_color_shader;
    }
    
    void GhostRenderLayer::SetUseCustomColorShader(bool on) noexcept
    {
        s_use_custom_color_shader = on;
        if (InstanceExists())
        {
            s_instance->m_render_pipeline.SetUseCustomGhostColorShader(on);
        }
    }

    void GhostRenderLayer::SetCurrentReplay(const std::optional<Replay>& replay) noexcept
    {
        s_replay_to_race_against = replay;

        s_replay_ghost_racing_line = [&replay]() -> std::optional<std::vector<CoreEngine::DrawLines3D_RenderPipeline::LineVertex>> {
            if (!replay.has_value())
                return std::nullopt;

            const std::vector<Replay::Frame>& frames = replay->GetFrameVectorConstReference();
            std::vector<CoreEngine::DrawLines3D_RenderPipeline::LineVertex> out;
            out.reserve(frames.size() * 2);

            auto HSVtoRGB = [](float h, float s, float v) -> glm::vec3 {
                h = fmod(h, 360.0f);
                float c = v * s;
                float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
                float m = v - c;
                glm::vec3 rgb;
                if (h < 60)       rgb = {c, x, 0};
                else if (h < 120) rgb = {x, c, 0};
                else if (h < 180) rgb = {0, c, x};
                else if (h < 240) rgb = {0, x, c};
                else if (h < 300) rgb = {x, 0, c};
                else              rgb = {c, 0, x};
                return rgb + glm::vec3(m);
            };

            float rainbow_length = 60.0f;
            float distance_accum = 0.0f;

            for (size_t i = 0; i < frames.size() - 1; ++i)
            {
                glm::vec3 p0 = frames[i].m_racer_state.GetExtractedPosition();
                glm::vec3 p1 = frames[i + 1].m_racer_state.GetExtractedPosition();

                float segment_length = glm::length(p1 - p0);
                distance_accum += segment_length;

                float hue0 = fmod(distance_accum / rainbow_length * 360.0f, 360.0f);
                float hue1 = fmod((distance_accum + segment_length) / rainbow_length * 360.0f, 360.0f);

                glm::vec3 color0 = HSVtoRGB(hue0, 1.0f, 1.0f);
                glm::vec3 color1 = HSVtoRGB(hue1, 1.0f, 1.0f);

                out.emplace_back(p0, color0);
                out.emplace_back(p1, color1);
            }

            return out;
        }();
    }

    const std::optional<Replay>& GhostRenderLayer::GetCurrentReplayConstRef() noexcept
    {
        return s_replay_to_race_against;
    }

    bool GhostRenderLayer::GetReplayPlaybackIsOn() noexcept
    {
        return s_race_against_replay;
    }

    void GhostRenderLayer::SetEnableReplayPlayback(bool on) noexcept
    {
        s_race_against_replay = on;
    }


    std::unique_ptr<CoreEngine::Basic_Model>& GhostRenderLayer::GetCurrentGhostModel() noexcept
    {
        return s_ghost_model;
    }

}