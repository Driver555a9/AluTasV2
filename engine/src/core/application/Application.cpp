#include "core/application/Application.h"

//Own includes
#include "core/application/ApplicationGlobalState.h"

#include "core/rendering/Renderer.h"

#include "core/event/WindowEvents.h"
#include "core/event/InputEvents.h"
#include "core/event/ApplicationStateEvents.h"

#include "core/utility/CommonUtility.h"
#include "core/utility/DebugUtility.h"
#include "core/utility/Timer.h"
#include "core/utility/Assert.h"
#include "core/utility/Performance.h"

//ImGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

namespace CoreEngine
{
    //Constructor
    Application::Application(Window&& window, ApplicationConfig config) : m_window(std::move(window)), m_stop_flag(false), m_original_config(config)
    {
        s_application_instance_ptr = this;
    }

    Application::~Application()
    {
        Application::OnCleanup();
    }

    void Application::OnCleanup() noexcept
    {
        glfwTerminate();
        s_application_instance_ptr = nullptr;
    }

    void Application::Run() noexcept
    {
        m_stop_flag = false;
        GLFWwindow* window_ptr = m_window.GetRawPtr();

        Renderer::InitializeImGui(window_ptr);
        ImGuiIO& io = ImGui::GetIO();

        Timer frame_timer {};

        while (! m_stop_flag)
        {
            ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("Main Loop()    ");
            //////////////////////////////////////////////// 
            //--------- Updating
            ////////////////////////////////////////////////
            #ifdef _WIN32
            if (m_window.GetIsClickthrough())
            {
                const bool mouse_over_imgui = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered() || io.WantCaptureMouse;
                m_window.SetClickthrough(! mouse_over_imgui);
            }
            #endif

            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("OnUpdate()     ");
                //from 0.001 milisec - 100 milisec
                m_frame_delta_time = std::clamp<Units::MicroSecond>(frame_timer.GetElapsedAndRestart<Units::MicroSecond>(), Units::MicroSecond(1L), Units::MicroSecond(100'000L));
                for (std::unique_ptr<Basic_Layer>& layer : m_layers)
                {
                    layer->OnUpdate(m_frame_delta_time);
                }
            }

            //////////////////////////////////////////////// 
            //--------- Rendering
            //////////////////////////////////////////////// 
            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("OnRender()     ");
                Renderer::BeginFrame(window_ptr);
                for (std::unique_ptr<Basic_Layer>& layer : m_layers)
                {
                    layer->OnRender();
                }
            }

            //////////////////////////////////////////////// 
            //--------- GUI Rendering
            //////////////////////////////////////////////// 
            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("OnImGuiRender()");
                Renderer::BeginImGuiFrame();
                for (std::unique_ptr<Basic_Layer>& layer : m_layers)
                {
                    layer->OnImGuiRender();
                }
                Renderer::FinalizeImGuiFrame();
            }

            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("FinalizeFrame()");
                Renderer::FinalizeFrame(window_ptr);
            }   
            //////////////////////////////////////////////// 
            //--------- Events
            //////////////////////////////////////////////// 
            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("Events()       ");
                if (m_original_config.m_use_glfw_await_events)
                {
                    glfwWaitEvents();
                }
                else 
                {
                    glfwPollEvents();
                }
            }
        }
        
        glfwHideWindow(window_ptr);
        Renderer::ShutdownImGui();
    }

    void Application::Stop() noexcept
    {
        m_stop_flag = true;
        RaiseEvent(ApplicationShutdownEvent {});
    }

    //--------------- Static callbacks
    void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)        s_application_instance_ptr->RaiseEvent(KeyPressedEvent  {key} );
        else if (action == GLFW_RELEASE) s_application_instance_ptr->RaiseEvent(KeyReleasedEvent {key} );
    }

    void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS)        s_application_instance_ptr->RaiseEvent(MousePressedEvent  {button, CommonUtility::GetMousePosition(window)});
        else if (action == GLFW_RELEASE) s_application_instance_ptr->RaiseEvent(MouseReleasedEvent {button, CommonUtility::GetMousePosition(window)});
    }

    void Application::MouseMovedCallback(GLFWwindow* window, double x_pos, double y_pos)
    {
        s_application_instance_ptr->RaiseEvent(MouseMovedEvent{x_pos, y_pos});
    }

    void Application::MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset)
    {
        s_application_instance_ptr->RaiseEvent(MouseScrolledEvent {x_offset, y_offset} );
    }

    void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        glfwMakeContextCurrent(window);
        glViewport(0, 0, width, height);

        s_application_instance_ptr->RaiseEvent(FramebufferResizeEvent {width, height});
    }

    void Application::WindowCloseCallback(GLFWwindow* window)
    {
        s_application_instance_ptr->RaiseEvent(WindowCloseEvent {});
        s_application_instance_ptr->Stop();
    }

    //--------------- Static initializer
    Application Application::Create(const ApplicationConfig& config)
    {
        if (s_application_instance_ptr)
        {
            ENGINE_ASSERT(false && "At Application::Create() called multiple times. Only one Application instance is allowed.");
        }

        if (! glfwInit())
        {
            Application::OnCleanup();
            throw std::runtime_error("At Application::Create(): failed to initialize GLFW.");
        }

    //------------ Initializaion of dependencies
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, std::max<int>(8, config.m_MSAA_sample_count));
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, config.m_transparent_click_through_window);
        glfwWindowHint(GLFW_VISIBLE, ! config.m_launch_with_hidden_window);

        Window::WindowCreationConfig window_config;
        window_config.m_relative_size               = config.m_relative_window_size;
        window_config.m_is_windowed_fullscren       = config.m_borderless_fullscreen;
        window_config.m_is_transparent_clickthrough = config.m_transparent_click_through_window;
        window_config.m_title                       = config.m_application_name;

        CoreEngine::Window window {window_config};
        GLFWwindow* const WINDOW_RAW_PTR = window.GetRawPtr();
        if (! WINDOW_RAW_PTR)
        {
            Application::OnCleanup();
            throw std::runtime_error("At Application::Create(): failed to create GLFW Window.");
        }

        glfwMakeContextCurrent(WINDOW_RAW_PTR);

        if (! gladLoadGL(glfwGetProcAddress)) 
        { 
            Application::OnCleanup();
            throw std::runtime_error("At Application::Create(): failed to initialize GLAD.");
        }

    //------------ OpenGL settings
        glClearColor(0.f, 0.f, 0.f, 1.f);

        if (config.m_transparent_click_through_window)
        {
            glClearColor(0.f, 0.f, 0.f, 0.f);
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        int framebuff_width, framebuff_height;
        glfwGetFramebufferSize(WINDOW_RAW_PTR, &framebuff_width, &framebuff_height);
        glViewport(0, 0, framebuff_width, framebuff_height);

        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDepthFunc(GL_GREATER);
        glClearDepth(0.0);
        glEnable(GL_DEPTH_TEST);

        glFrontFace(GL_CCW); 
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);

        if (config.m_MSAA_sample_count > 0) 
        {
            glEnable(GL_MULTISAMPLE);
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }

    //------------ Enable debug messages on console
        if (config.m_debug_launch_with_console)
        {
            CoreEngine::DebugUtility::ForceInitConsole();
            CoreEngine::DebugUtility::PrintHardwareInfo();
            CoreEngine::DebugUtility::EnableDebugMessages();
        }
        else 
        {
            CoreEngine::DebugUtility::ForceCloseConsole();
        }

    //------------ Set GLFW Callbacks
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::KeyCallback) == ApplicationConfig::CallbackDisableFlags::NONE)
        {
            glfwSetKeyCallback (WINDOW_RAW_PTR, &Application::KeyCallback);
        }
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::FramebufferResizeCallback) == ApplicationConfig::CallbackDisableFlags::NONE )
        {
            glfwSetFramebufferSizeCallback (WINDOW_RAW_PTR, &Application::FramebufferResizeCallback);
        }
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::MouseButtonCallback) == ApplicationConfig::CallbackDisableFlags::NONE )
        {
            glfwSetMouseButtonCallback (WINDOW_RAW_PTR, &Application::MouseButtonCallback);
        }
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::MouseMovedCallback) == ApplicationConfig::CallbackDisableFlags::NONE )
        {
            glfwSetCursorPosCallback (WINDOW_RAW_PTR, &Application::MouseMovedCallback);
        }
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::MouseScrollCallback) == ApplicationConfig::CallbackDisableFlags::NONE )
        {
            glfwSetScrollCallback (WINDOW_RAW_PTR, &Application::MouseScrollCallback);
        }
        if ( (config.m_disable_callback_flags & ApplicationConfig::CallbackDisableFlags::WindowCloseCallback) == ApplicationConfig::CallbackDisableFlags::NONE )
        {
            glfwSetWindowCloseCallback (WINDOW_RAW_PTR, &Application::WindowCloseCallback);
        }

        s_vsync_is_on = config.m_enable_vsync;
        glfwSwapInterval(s_vsync_is_on);

        glfwSetInputMode(WINDOW_RAW_PTR, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

        Renderer::s_imgui_config_flags |= config.m_imgui_config_flags;

        return Application {std::move(window), config};
    }

    
}