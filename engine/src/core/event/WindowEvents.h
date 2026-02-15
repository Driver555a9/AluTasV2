#pragma once

#include "core/event/Event.h"

namespace CoreEngine
{
    class WindowCloseEvent : public Basic_Event
    {
        public:
            explicit WindowCloseEvent() noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::WindowClose; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class FramebufferResizeEvent : public Basic_Event 
    {
        protected:
            const int m_width, m_height;

        public:
            explicit FramebufferResizeEvent(const int width, const int height) noexcept;

            [[nodiscard]] int GetWidth()  const noexcept;
            [[nodiscard]] int GetHeight() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::FramebufferResize; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };
}