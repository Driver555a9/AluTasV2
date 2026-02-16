#pragma once

#include <string>

namespace CoreEngine
{
    enum class EventType
    {
        None,
        WindowClose, FramebufferResize,
        MousePressed, MouseReleased, MouseMoved, MouseScrolled,
        KeyPressed, KeyReleased,
        ApplicationShutdown, 
        Custom
    };

    class Basic_Event
    {
        protected:
            bool m_is_handled = false;
            EventType m_type = EventType::None;

            explicit Basic_Event(const EventType type) : m_type(type) {}
            
        public:

            virtual ~Basic_Event() = default;
            
            [[nodiscard]] EventType GetEventType()        const noexcept { return m_type; };
            [[nodiscard]] virtual std::string ToString()  const noexcept = 0;

            [[nodiscard]] inline bool GetIsHandled() const noexcept { return m_is_handled; }
            inline void MarkAsHandled() noexcept { m_is_handled = true; }
    };
}

