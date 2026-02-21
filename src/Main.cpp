#include "core/application/Application.h"

#include "tas/layers/TasLayer.h"

int main()
{

    constexpr CoreEngine::Application::ApplicationConfig application_config 
    {
        .m_enable_vsync                     = true,
        .m_debug_launch_with_console        = true,
        .m_use_glfw_await_events            = false
    };

    using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

    const CoreEngine::Window::WindowCreationConfig window_config
    {
        .m_title                       = "Asphalt Tool",
        .m_relative_size               = {500.0f / 1920.0f, 800.0f / 1080.0f},
        .m_callback_disable_flags      = static_cast<Cdis>(Cdis::KeyCallback | Cdis::MouseButtonCallback | Cdis::MouseMovedCallback | Cdis::MouseScrollCallback),
        .m_imgui_flags                 = {},
        .m_MSAA_sample_count           = 0,
        .m_is_windowed_fullscren       = false,
        .m_has_transparent_framebuffer = false
    };

    CoreEngine::Application app = CoreEngine::Application::Create(application_config);
    app.QueueCreateWindowAndPushLayer<AsphaltTas::TasLayer>(window_config);
    app.Run();
}
