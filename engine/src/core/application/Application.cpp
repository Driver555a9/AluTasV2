#include "core/application/Application.h"

//Own includes

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
    Application::Application(ApplicationConfig config) noexcept : m_original_config(config), m_vsync_is_on(config.m_enable_vsync)
    {
        s_application_instance_ptr = this;
    }

    Application::~Application()
    {
        m_window_layer_stacks.clear();
        s_application_instance_ptr = nullptr;
        glfwTerminate();
    }

    void Application::Run() noexcept
    {
        m_stop_flag = false;

        Timer frame_timer {};

        while (! m_stop_flag)
        {
            ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("Main Loop()  ");

            constexpr Units::MicroSecond min_dt (1L);
            constexpr Units::MicroSecond max_dt (100'000L);
            m_frame_delta_time = std::clamp<Units::MicroSecond>(frame_timer.GetElapsedAndRestart<Units::MicroSecond>(), min_dt, max_dt);

            for (std::unique_ptr<WindowLayerStack>& wls : m_window_layer_stacks)
            {
                //////////////////////////////////////////////// 
                //--------- Updating
                //////////////////////////////////////////////// 
                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME( wls->m_window_ptr->GetTitle() + std::string(" : OnUpdate()     ") );
                    for (std::unique_ptr<Basic_Layer>& layer : wls->m_layer_stack)
                    {
                        layer->OnUpdate(m_frame_delta_time);
                    }
                }

                if (! wls->m_window_ptr->IsVisible())
                {
                    continue;
                }

                //////////////////////////////////////////////// 
                //--------- Rendering
                //////////////////////////////////////////////// 
                wls->m_window_ptr->BeginFrame();
                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME( wls->m_window_ptr->GetTitle() + std::string(" : OnRender()     ") );
                    for (std::unique_ptr<Basic_Layer>& layer : wls->m_layer_stack)
                    {
                        layer->OnRender();
                    }
                }
                
                //////////////////////////////////////////////// 
                //--------- Gui Rendering
                //////////////////////////////////////////////// 
                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME( wls->m_window_ptr->GetTitle() + std::string(" : OnImGuiRender()") );
                    wls->m_window_ptr->BeginImGuiFrame();
                    for (std::unique_ptr<Basic_Layer>& layer : wls->m_layer_stack)
                    {
                        layer->OnImGuiRender();
                    }
                    wls->m_window_ptr->FinishImGuiFrame();
                }

                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME(wls->m_window_ptr->GetTitle() + std::string(" : FinishFrame()  "));
                    wls->m_window_ptr->FinishFrame();
                }
            }

            //////////////////////////////////////////////// 
            //--------- Events
            //////////////////////////////////////////////// 
            {
                if (m_original_config.m_use_glfw_await_events)
                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("AwaitEvents()");
                    glfwWaitEvents();
                }
                else 
                {
                    ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("PollEvents() ");
                    glfwPollEvents();
                }
            }

            //////////////////////////////////////////////// 
            //--------- Deleting windows
            //////////////////////////////////////////////// 
            {
                ENGINE_PERFORMANCE_MEASURE_SCOPE_TIME("Add Layers() ");
                for (auto rit = m_window_layer_stacks_to_delete_next_frame.rbegin(); rit != m_window_layer_stacks_to_delete_next_frame.rend(); ++rit)
                {
                    const auto handle  = *rit;
                    if (handle == Window::Handle::INVALID)
                    {
                        ENGINE_DEBUG_PRINT("Attempted to delete window with invalid handle: " << handle);
                        continue;
                    }
                    const size_t index = FindWindowLayerStackIndexFromWindowHandle(handle);

                    m_window_layer_stacks.erase(m_window_layer_stacks.begin() + index);
                }
                m_window_layer_stacks_to_delete_next_frame.clear();

                //////////////////////////////////////////////// 
                //--------- Adding windows
                //////////////////////////////////////////////// 
                for (std::pair<Window::WindowCreationConfig, std::vector<Application::LayerFactory>>& request : m_window_creations_to_add_next_frame)
                {
                    m_window_layer_stacks.emplace_back(std::make_unique<WindowLayerStack>(std::make_unique<Window>(request.first)));
                    Window::Handle new_handle = m_window_layer_stacks.back()->m_window_ptr->GetHandle();
                    WindowLayerStack* new_wls = m_window_layer_stacks.back().get();

                    glfwMakeContextCurrent(new_wls->m_window_ptr->GetGLFWwindow());
                    glfwSwapInterval(m_vsync_is_on);

                    for (LayerFactory& factory : request.second)
                    {
                        std::unique_ptr<Basic_Layer> layer = factory(new_handle);
                        new_wls->m_layer_stack.push_back(std::move(layer));
                    }
                }
                m_window_creations_to_add_next_frame.clear();
            }
        }
    }

    void Application::Stop() noexcept
    {
        m_stop_flag = true;
        
        for (std::unique_ptr<CoreEngine::Application::WindowLayerStack>& window_layerstack : m_window_layer_stacks)
        {
            RaiseEvent(window_layerstack->m_window_ptr->GetHandle(), ApplicationShutdownEvent {});
        }
    }

    void Application::QueueDeleteWindowLayerStack(Window::Handle group_handle) noexcept
    {
        m_window_layer_stacks_to_delete_next_frame.push_back(group_handle);
    }

    Window* Application::GetWindowPtr(Window::Handle group_handle) noexcept
    {
        return m_window_layer_stacks[FindWindowLayerStackIndexFromWindowHandle(group_handle)]->m_window_ptr.get();
    }

    size_t Application::GetAmountWindows() const noexcept
    {
        return m_window_layer_stacks.size();
    }

    void Application::SetVsync(bool on) noexcept
    {
        m_vsync_is_on = on;
        GLFWwindow* context = glfwGetCurrentContext();
        for (std::unique_ptr<WindowLayerStack>& wls : m_window_layer_stacks)
        {
            glfwMakeContextCurrent(wls->m_window_ptr->GetGLFWwindow());
            glfwSwapInterval(on);
        }
        glfwMakeContextCurrent(context);
    }

    bool Application::GetVsyncIsOn() const noexcept
    {
        return m_vsync_is_on;
    }  

    Units::MicroSecond Application::GetLastFrameTime() const noexcept
    {
        return m_frame_delta_time;
    }
    
    /////////////////////////////////////////////// 
    // Application creation
    //////////////////////////////////////////////// 
    Application Application::Create(const ApplicationConfig& config)
    {
        if (s_application_instance_ptr)
        {
            ENGINE_ASSERT(false && "At Application::Create() called multiple times. Only one Application instance is allowed.");
        }
        if (! glfwInit())
        {
            throw std::runtime_error("At Application::Create(): failed to initialize GLFW.");
        }
        
        glfwWindowHint(GLFW_VISIBLE, false);
        GLFWwindow* dummy = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        glfwWindowHint(GLFW_VISIBLE, true);
        ENGINE_ASSERT(dummy && "Failed to create dummy GLFW window for GLAD");

        glfwMakeContextCurrent(dummy);
        ENGINE_ASSERT(gladLoadGL(glfwGetProcAddress) && "Failed to load GL");

        glfwSwapInterval(config.m_enable_vsync);

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

        glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(dummy);

        return Application {config};
    }

    Application* Application::Get() noexcept
    {
        ENGINE_ASSERT(s_application_instance_ptr && "Can not call Get() if no instance of application exists.");
        return s_application_instance_ptr;
    }

    //////////////////////////////////////////////// 
    //--------- Glfw callbacks
    //////////////////////////////////////////////// 
    void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        if (action == GLFW_PRESS)        s_application_instance_ptr->RaiseEvent(handle, KeyPressedEvent  {key} );
        else if (action == GLFW_RELEASE) s_application_instance_ptr->RaiseEvent(handle, KeyReleasedEvent {key} );
    }

    void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        if (action == GLFW_PRESS)        s_application_instance_ptr->RaiseEvent(handle, MousePressedEvent  {button, CommonUtility::GetMousePosition(window)});
        else if (action == GLFW_RELEASE) s_application_instance_ptr->RaiseEvent(handle, MouseReleasedEvent {button, CommonUtility::GetMousePosition(window)});
    }

    void Application::MouseMovedCallback(GLFWwindow* window, double x_pos, double y_pos)
    {
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        s_application_instance_ptr->RaiseEvent(handle, MouseMovedEvent{x_pos, y_pos});
    }

    void Application::MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset)
    {
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        s_application_instance_ptr->RaiseEvent(handle, MouseScrolledEvent {x_offset, y_offset} );
    }

    void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        glfwMakeContextCurrent(window);
        glViewport(0, 0, width, height);
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        s_application_instance_ptr->RaiseEvent(handle, FramebufferResizeEvent {width, height});
    }

    void Application::WindowCloseCallback(GLFWwindow* window)
    {
        const Window::Handle handle = s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window);
        s_application_instance_ptr->RaiseEvent(handle, WindowCloseEvent {});

        s_application_instance_ptr->QueueDeleteWindowLayerStack(s_application_instance_ptr->FindWindowHandleFromGlfwWindow(window));

        if (s_application_instance_ptr->m_window_layer_stacks.size() == 1)
        {
            s_application_instance_ptr->Stop();
        }
    }
    

    //////////////////////////////////////////////// 
    //--------- Private methods
    //////////////////////////////////////////////// 
    Window::Handle Application::FindWindowHandleFromGlfwWindow(GLFWwindow* window) const noexcept
    {
        for (size_t index{}; index < m_window_layer_stacks.size(); ++index)
        {
            if (m_window_layer_stacks[index]->m_window_ptr->GetGLFWwindow() == window)
                return m_window_layer_stacks[index]->m_window_ptr->GetHandle();
        }
        ENGINE_ASSERT(false && "Failed to fetch Window Handle from glfw window*: GLFWwindow not part of window layer stack.");
    }

    size_t Application::FindWindowLayerStackIndexFromWindowHandle(Window::Handle handle) const noexcept
    {
        for (size_t index{}; index < m_window_layer_stacks.size(); ++index)
        {
            if (m_window_layer_stacks[index]->m_window_ptr->GetHandle() == handle)
                return index;
        }

        ENGINE_ASSERT(false && "Failed to find window layer stack index from Handle: GLFWwindow not part of window layer stack.");
    }
}