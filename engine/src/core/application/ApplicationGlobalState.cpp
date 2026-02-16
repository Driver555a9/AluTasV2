#include "core/application/Application.h"
#include "core/application/ApplicationGlobalState.h"

#include "core/utility/CommonUtility.h"

namespace CoreEngine
{
    template <typename TGet>
    auto GlobalGet() -> TGet::type
    {
        if constexpr (std::same_as<TGet, GlobalGet_FramebufferSize>)
        {
            return CommonUtility::GetFramebufferSize(Application::s_application_instance_ptr->m_window.GetRawPtr());
        }
        else if constexpr (std::same_as<TGet, GlobalGet_AspectRatio>)
        {
            const auto [width, height] = CommonUtility::GetFramebufferSize(Application::s_application_instance_ptr->m_window.GetRawPtr());
            return static_cast<float>(width) / height;
        }
        else if constexpr (std::same_as<TGet, GlobalGet_ApplicationName>)
        {
            return glfwGetWindowTitle(Application::s_application_instance_ptr->m_window.GetRawPtr());
        }
        else if constexpr (std::same_as<TGet, GlobalGet_VsyncIsOn>)
        {
            return Application::s_vsync_is_on;
        }
        else if constexpr (std::same_as<TGet, GlobalGet_DeltaTimeMicros>)
        {
            return Units::Convert<Units::MicroSecond>(Application::s_application_instance_ptr->m_frame_delta_time);
        }
        else if constexpr (std::same_as<TGet, GlobalGet_DeltaTimeSeconds>)
        {
            return Units::Convert<Units::Second>(Application::s_application_instance_ptr->m_frame_delta_time);
        }
        else 
        {
            static_assert(sizeof(TGet) == 0 && "At GlobalGet(): Unrecognized type to get.");
        }
    }

    template GlobalGet_FramebufferSize::type  GlobalGet<GlobalGet_FramebufferSize>();
    template GlobalGet_AspectRatio::type      GlobalGet<GlobalGet_AspectRatio>();
    template GlobalGet_ApplicationName::type  GlobalGet<GlobalGet_ApplicationName>();
    template GlobalGet_VsyncIsOn::type        GlobalGet<GlobalGet_VsyncIsOn>();
    template GlobalGet_DeltaTimeMicros::type  GlobalGet<GlobalGet_DeltaTimeMicros>();
    template GlobalGet_DeltaTimeSeconds::type GlobalGet<GlobalGet_DeltaTimeSeconds>();

//-------------------------

    template <typename TSet, typename Arg>
    void GlobalSet(Arg&& arg)
    {
        if constexpr (std::same_as<TSet, GlobalSet_VsyncIsOn>)
        {
            const bool value = static_cast<bool>(std::forward<Arg>(arg));
            Application::s_vsync_is_on = value;
            glfwSwapInterval(value);
            return;
        }
        else if constexpr (std::same_as<TSet, GlobalSet_RaiseEvent> && std::is_base_of_v<Basic_Event, std::remove_cvref_t<Arg>>)
        {
            Application::s_application_instance_ptr->RaiseEvent<std::remove_cvref_t<Arg>>(std::forward<Arg>(arg));
            return;
        }
        else 
        {
            static_assert(sizeof(TSet) == 0 && "At GlobalSet(Arg&& arg): Unrecognized type to set.");
        }
    }

    #define INSTANTIATE_GLOBALSET(TSet, Arg) \
    template void GlobalSet<TSet, Arg>(Arg&&); \
    template void GlobalSet<TSet, Arg&>(Arg&); \
    template void GlobalSet<TSet, const Arg&>(const Arg&);

    INSTANTIATE_GLOBALSET(GlobalSet_VsyncIsOn, bool)
    template void GlobalSet<GlobalSet_RaiseEvent, Basic_Event>(Basic_Event&&);

    #undef INSTANTIATE_GLOBALSET

    template <typename TSet>
    void GlobalSet()
    {
        if constexpr (std::same_as<TSet, GlobalSet_StopApplication>)
        {
            Application::s_application_instance_ptr->Stop();
        }
        else 
        {
            static_assert(sizeof(TSet) == 0 && "At GlobalSet(void): Unrecognized type to set.");
        }
    }

    template void GlobalSet<GlobalSet_StopApplication>();
}