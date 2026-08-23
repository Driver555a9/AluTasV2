#include "layer/SpeedometerLayer.h"

#include "Communication.h"
#include "core/application/Application.h"
#include "core/utility/Assert.h"
#include "core/event/InputEvents.h"
#include "core/event/EventDispatcher.h"

#include "globalstate/AsphaltDllManager.h"

#include "imgui.h"
#include "layer/GuiStyle.h"

#include "common/RacerState.h"
#include "common/Utility.h"

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

        dispatcher.Dispatch<CoreEngine::MouseReleasedEvent>([this]([[maybe_unused]] CoreEngine::MouseReleasedEvent& e) -> bool {
            m_left_mouse_pressed_after_unlock_disable_gui_input = false;
            return true;
        });
    }

    void SpeedometerLayer::OnUpdate([[maybe_unused]] CoreEngine::Units::MicroSecond dt) noexcept
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
                               
        PUSH_SCOPED_STYLE_COLOR(ImGuiCol_WindowBg, m_is_locked ? m_bg_color : GuiStyle::COLOR_BLACK);

        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowBorderSize, 0.0f);
        PUSH_SCOPED_STYLE_VAR(ImGuiStyleVar_WindowRounding, 0.0f);
        
        if (! ImGui::Begin("Speedometer", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        {  // Scope to delete scoped styles
            ImGui::SetWindowFontScale(m_font_size);

            std::optional<ComDllOut::DllStateOut> out = AsphaltDllManager::GetDllStateOutCopy();
            
            if (out.has_value() && out->m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE)
            {
                RacerState racer(out->m_racer_state);
                const float real_ms = glm::length(racer.GetVelocityOpenGL_XYZ());
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                ImGui::Text("%3.1f", real_ms * 3.6f);

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                //ImGui::Text("Fake: %3.1f", Utility::ConvertRealSpeedKmhToFakeSpeedKmh(real_ms * 3.6f));

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_WHITE);
                ImGui::Text("%u", out->m_replay_inputs.m_race_frame_tick);
            }
            else 
            {
                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_GREEN);
                ImGui::TextUnformatted("XXX.X");

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_RED);
                //ImGui::TextUnformatted("Fake: XXX.X");

                PUSH_SCOPED_STYLE_COLOR(ImGuiCol_Text, GuiStyle::COLOR_WHITE);
                ImGui::TextUnformatted("XXX");
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

                    ImGui::ColorPicker4("BG Color", &m_bg_color.x);
                }
            }
        }

        ImGui::End();

    }

    void SpeedometerLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one SpeedometerLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "Speedometer ",
            .m_relative_size               = {500.0f / 1920.0f, 600.0f / 1080.0f},
            .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 0,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = true,
            .m_is_clickthrough             = false
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