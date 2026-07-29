#include "core/application/Application.h"

#include "layer/MainLayer.h"

#include "layer/TrackViewerLayer.h"

int main()
{
    constexpr CoreEngine::Application::ApplicationConfig application_config 
    {
        .m_enable_vsync                     = true,
        .m_debug_launch_with_console        = true,
        .m_use_glfw_await_events            = false
    };

    CoreEngine::Application app = CoreEngine::Application::Create(application_config);
    AsphaltTas::MainLayer::CreateInstance();
    app.Run();
}
