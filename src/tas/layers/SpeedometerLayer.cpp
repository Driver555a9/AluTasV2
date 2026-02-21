#include "tas/layers/SpeedometerLayer.h"

#include "core/application/Application.h"
#include "core/utility/Assert.h"
#include "core/event/InputEvents.h"
#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"

#include "tas/layers/GuiStyle.h"

#include "tas/common/RacerState.h"
#include "tas/common/Utility.h"

#include "tas/servicethreads/ReadCurrentStateService.h"

#include "glad/gl.h"

namespace AsphaltTas
{
    SpeedometerLayer::SpeedometerLayer(CoreEngine::Window::Handle handle) noexcept : CoreEngine::Basic_Layer(handle)
    {
        s_instance = this;
    }

    SpeedometerLayer::~SpeedometerLayer() noexcept
    {
        s_instance = nullptr;
    }   
     
    void SpeedometerLayer::OnEvent(CoreEngine::Basic_Event& e) noexcept
    {
        CoreEngine::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<CoreEngine::MousePressedEvent>([this](CoreEngine::MousePressedEvent& e) -> bool {
            if (m_is_locked && e.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT)
            {
                OnUnlock();
                m_left_mouse_pressed_after_unlock_disable_gui_input = true;
            }
            return true;
        });

        dispatcher.Dispatch<CoreEngine::MouseReleasedEvent>([this](CoreEngine::MouseReleasedEvent& e) -> bool {
            m_left_mouse_pressed_after_unlock_disable_gui_input = false;
            return true;
        });
    }

    void SpeedometerLayer::OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept
    {

    }

    void SpeedometerLayer::OnRender() noexcept
    {

    }

    void SpeedometerLayer::OnImGuiRender() noexcept
    {
        ImVec2 display = ImGui::GetIO().DisplaySize;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(display);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                               | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
                               
        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_WindowBg, m_is_locked ? GuiStyle::COLOR_TRANSPARENT : GuiStyle::COLOR_BLACK);

        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);
        
        if (! ImGui::Begin("Free Flight", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        {  // Scope to delete scoped styles
            ImGui::SetWindowFontScale(m_font_size);

            std::optional<RacerState> state_now = ReadCurrentStateService::GetCurrentRacerState();

            if (state_now.has_value())
            {
                const float real_ms = glm::length(state_now->GetVelocity());
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                ImGui::Text("Real: %3.1f", real_ms * 3.6f);

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                ImGui::Text("Fake: %3.1f", Utility::ConvertRealSpeedKmhToFakeSpeedKmh(real_ms * 3.6f));
            }
            else 
            {
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                ImGui::Text("Real: XXX.X");

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                ImGui::Text("Fake: XXX.X");
            }

            ImGui::SetWindowFontScale(1.0f);

            if (! m_left_mouse_pressed_after_unlock_disable_gui_input)
            {
                if (! m_is_locked)
                {
                    PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Button, GuiStyle::COLOR_GREEN);
                    if (ImGui::Button("Lock"))
                    {
                        OnLock();
                    }
                    
                    ImGui::SameLine();
                    ImGui::SliderFloat("Font Size", &m_font_size, 0.1f, 10.0f);
                }
            }
        }

        ImGui::End();

    }

    void SpeedometerLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one SpeedometerLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        const CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "Speedometer ",
            .m_relative_size               = {500.0f / 1920.0f, 300.0f / 1080.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_windowed_fullscren       = false,
            .m_has_transparent_framebuffer = true
        };
        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<SpeedometerLayer>(config);
    }

    bool SpeedometerLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void SpeedometerLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }

    void SpeedometerLayer::OnLock() noexcept
    {
        m_is_locked = true;
        GLFWwindow* window = CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetGLFWwindow();
        glfwMakeContextCurrent(window);
        glfwSetWindowAttrib(window, GLFW_DECORATED, false);
        glfwSetWindowAttrib(window, GLFW_FLOATING, true);
        glClearColor(0, 0, 0, 0);
    }

    void SpeedometerLayer::OnUnlock() noexcept
    {
        m_is_locked = false;
        GLFWwindow* window = CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetGLFWwindow();
        glfwMakeContextCurrent(window);
        glfwSetWindowAttrib(window, GLFW_DECORATED, true);
        glfwSetWindowAttrib(window, GLFW_FLOATING, false);
        glClearColor(0, 0, 0, 1);
    }

}