#include "core/event/WindowEvents.h"

//std
#include <format>

namespace CoreEngine
{
//------------- WindowCloseEvent
    WindowCloseEvent::WindowCloseEvent() noexcept : Basic_Event(EventType::WindowClose) {}

    std::string WindowCloseEvent::ToString() const noexcept { return "WindowCloseEvent"; }

//------------- FramebufferResizeEvent
    FramebufferResizeEvent::FramebufferResizeEvent(const int width, const int height) noexcept
    : Basic_Event(EventType::FramebufferResize), m_width(width), m_height(height) {}

    int FramebufferResizeEvent::GetWidth()  const noexcept { return m_width; }
    int FramebufferResizeEvent::GetHeight() const noexcept { return m_height; }

    std::string FramebufferResizeEvent::ToString()   const noexcept { return std::format("WindowResizeEvent: {}, {}", m_width, m_height); }
}