#pragma once

//std imports
#include <memory>
#include <vector>
#include <iostream>

//personal imports
#include "core/layer/Layer.h"

#include "core/event/Event.h"

#include "core/application/Window.h"

#include "core/utility/Units.h"

#include "imgui/imgui.h"

namespace CoreEngine
{
    class Application final
    {
    public:
        //////////////////////////////////////////////// 
        //--------- Setup config
        //////////////////////////////////////////////// 
        struct ApplicationConfig
        {
            enum CallbackDisableFlags : int
            {
                NONE                        = 0,
                KeyCallback                 = 1 << 0,
                MouseButtonCallback         = 1 << 1,
                MouseMovedCallback          = 1 << 2,
                MouseScrollCallback         = 1 << 3,
                FramebufferResizeCallback   = 1 << 4,
                WindowCloseCallback         = 1 << 5
            };
            
            std::string             m_application_name                 {"generic"};
            std::pair<float, float> m_relative_window_size             {1.0f, 1.0f};
            std::uint8_t            m_MSAA_sample_count                {0};
            CallbackDisableFlags    m_disable_callback_flags           {CallbackDisableFlags::NONE};
            ImGuiConfigFlags        m_imgui_config_flags               {};
            bool                    m_enable_vsync                     {false};
            bool                    m_borderless_fullscreen            {false};
            bool                    m_transparent_click_through_window {false};
            bool                    m_launch_with_hidden_window        {false};
            bool                    m_debug_launch_with_console        {true};
            bool                    m_use_glfw_await_events            {false};
        };

        /////////////////////////////////////////////// 
        //--------- Application Instance Creation - Once instance goes out of scope, OnCleanup() is called in ~Application() - then Create() may be called again
        //////////////////////////////////////////////// 
        [[nodiscard]] static Application Create(const ApplicationConfig& config);

        //////////////////////////////////////////////// 
        //--------- Glfw callbacks
        //////////////////////////////////////////////// 
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void MouseMovedCallback(GLFWwindow* window, double x_pos, double y_pos);
        static void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);
        static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
        static void WindowCloseCallback(GLFWwindow* window);

        //////////////////////////////////////////////// 
        //--------- Public Methods
        //////////////////////////////////////////////// 
        ~Application();
        void Run() noexcept;
        void Stop() noexcept;

        //////////////////////////////////////////////// 
        //--------- Inline methods
        //////////////////////////////////////////////// 
        template <typename TEvent>
        requires (std::is_base_of_v<Basic_Event, TEvent>)
        inline void RaiseEvent(TEvent&& event) noexcept
        {
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it)
            {
                if (event.GetIsHandled())
                    break;

                (*it)->OnEvent(event);
            }
        }

        template <typename TLayer, typename... Args>
        requires (std::is_base_of_v<Basic_Layer, TLayer>)
        inline void PushLayer(Args&&... args) noexcept
        {
            m_layers.push_back(std::make_unique<TLayer>(std::forward<Args>(args)...));
        }

        [[nodiscard]] inline bool RemoveLayer(const size_t index) noexcept 
        {
            if (index >= m_layers.size()) [[unlikely]]
            {
                std::cout << "At CoreEngine::Application::RemoveLayer(): index is out of bounds!" << std::endl;
                return false;
            }
            m_layers.erase(m_layers.begin() + index);
            return true;
        }

        template <typename TLayer>
        requires (std::is_base_of_v<Basic_Layer, TLayer>)
        [[nodiscard]] inline bool SwapLayer(const size_t index) noexcept
        {
            if (index >= m_layers.size()) [[unlikely]]
            {
                std::cout << "At CoreEngine::Application::SwapLayer(): index is out of bounds!" << std::endl;
                return false;
            }
            m_layers[index] = std::make_unique<TLayer>();
            return true;
        }

    private:
        //////////////////////////////////////////////// 
        //--------- Member variables
        //////////////////////////////////////////////// 
        std::vector<std::unique_ptr<Basic_Layer>> m_layers;
        Window                                    m_window;
        bool                                      m_stop_flag;
        Units::MicroSecond                        m_frame_delta_time {0};
        ApplicationConfig                         m_original_config;
        
        //////////////////////////////////////////////// 
        //--------- Global State variables that can not trivially be gotten through dependencies
        //////////////////////////////////////////////// 
        static inline bool s_vsync_is_on = false;
        static inline Application* s_application_instance_ptr = nullptr;

        //////////////////////////////////////////////// 
        //--------- Constructor to be called by Create()
        //////////////////////////////////////////////// 
        explicit Application(Window&& window, ApplicationConfig config);
        
        static void OnCleanup() noexcept; ///Called in instance destructor, or if Create() fails

        //////////////////////////////////////////////// 
        //--------- Copy / Move policy 
        //////////////////////////////////////////////// 
        Application()                              = delete;
        Application(const Application&)            = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&)      = delete;
        Application(Application&&)                 = delete;

        //////////////////////////////////////////////// 
        //--------- Give Global get access to Application data
        ////////////////////////////////////////////////
        template <typename TGet>
        friend auto GlobalGet() -> TGet::type;

        template <typename TSet, typename Arg>
        friend void GlobalSet(Arg&&);

        template <typename TSet>
        friend void GlobalSet();
    };
}