#include "core/application/Application.h"

#include "tas/TasLayer.h"

int main()
{
    using Calldis = CoreEngine::Application::ApplicationConfig::CallbackDisableFlags;

    constexpr Calldis event_disable = static_cast<Calldis>(Calldis::KeyCallback | Calldis::MouseButtonCallback | Calldis::MouseMovedCallback | Calldis::MouseScrollCallback);

    const CoreEngine::Application::ApplicationConfig config 
    {
        .m_application_name                 = "Asphalt Tool",
        .m_window_size                      = {500, 900},
        .m_MSAA_sample_count                = 0,
        .m_disable_callback_flags           = event_disable,
        .m_imgui_config_flags               = static_cast<ImGuiConfigFlags>(ImGuiConfigFlags_DpiEnableScaleFonts | ImGuiConfigFlags_DpiEnableScaleViewports),
        .m_enable_vsync                     = true,
        .m_borderless_fullscreen            = false,
        .m_transparent_click_through_window = false,
        .m_launch_with_hidden_window        = false,
        .m_debug_launch_with_console        = true,
        .m_use_glfw_await_events            = false
    };

    CoreEngine::Application app = CoreEngine::Application::Create(config);
    app.PushLayer<AsphaltTas::TasLayer>();
    app.Run();
}
