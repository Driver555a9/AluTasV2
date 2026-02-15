#pragma once

//Own includes
#include "core/event/Event.h"

//std
#include <functional>

namespace CoreEngine
{
    class EventDispatcher
    {
    public:
        EventDispatcher(Basic_Event& event) : m_event(event) {}

        template <typename TEvent, typename TFunc>
        requires ( std::is_base_of_v<Basic_Event, TEvent> && std::invocable<TFunc, TEvent&> && std::same_as<std::invoke_result_t<TFunc, TEvent&>, bool> )
        inline bool Dispatch(TFunc&& func)
        {
            if (m_event.GetEventType() != TEvent::GetStaticType() || m_event.GetIsHandled())
                return false;

            if (std::invoke(std::forward<TFunc>(func), static_cast<TEvent&>(m_event)))
                m_event.MarkAsHandled();

            return true;
        }
        
    private:
        Basic_Event& m_event;
    };
}