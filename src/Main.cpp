#include "core/application/Application.h"

#include "tas/TasLayer.h"

int main()
{
    const CoreEngine::Application::ApplicationConfig config 
    {
        .m_application_name                 = "Asphalt Tool",
        .m_window_size                      = {700, 500},
        .m_MSAA_sample_count                = 8,
        .m_enable_vsync                     = true,
        .m_borderless_fullscreen            = false,
        .m_transparent_click_through_window = false,
        .m_launch_with_hidden_window        = true,
        .m_debug_launch_with_console        = true
    };

    CoreEngine::Application app = CoreEngine::Application::Create(config);
    app.PushLayer<AsphaltTas::TasLayer>();
    app.Run();
}
