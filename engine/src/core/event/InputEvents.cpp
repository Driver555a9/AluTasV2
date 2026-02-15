#include "core/event/InputEvents.h"

//std
#include <format>

namespace CoreEngine
{
//------------- MousePressedEvent
    MousePressedEvent::MousePressedEvent(const int button, const double x, const double y) 
    : Basic_Event(EventType::MousePressed), m_mouse_button(button), m_x_pos(x), m_y_pos(y) {}

    MousePressedEvent::MousePressedEvent(const int button, const std::pair<double, double>& pos) 
    : Basic_Event(EventType::MousePressed), m_mouse_button(button), m_x_pos(pos.first), m_y_pos(pos.second) {}

    double MousePressedEvent::GetX() const noexcept { return m_x_pos; }
    double MousePressedEvent::GetY() const noexcept { return m_y_pos; }
    int MousePressedEvent::GetMouseButton() const noexcept { return m_mouse_button; }

    std::string MousePressedEvent::ToString() const noexcept { return std::format("MousePressedEvent: pos: {}, {} button: {}", m_x_pos, m_y_pos, m_mouse_button); }

//------------- MouseReleasedEvent
    MouseReleasedEvent::MouseReleasedEvent(const int button, const double x, const double y) 
    : Basic_Event(EventType::MouseReleased), m_mouse_button(button), m_x_pos(x), m_y_pos(y) {}

    MouseReleasedEvent::MouseReleasedEvent(const int button, const std::pair<double, double>& pos) 
    : Basic_Event(EventType::MouseReleased), m_mouse_button(button), m_x_pos(pos.first), m_y_pos(pos.second) {}

    double MouseReleasedEvent::GetX() const noexcept { return m_x_pos; }
    double MouseReleasedEvent::MouseReleasedEvent::GetY() const noexcept { return m_y_pos; }
    int MouseReleasedEvent::GetMouseButton() const noexcept { return m_mouse_button; }

    std::string MouseReleasedEvent::ToString() const noexcept { return std::format("MouseReleasedEvent: pos: {}, {} button: {}", m_x_pos, m_y_pos, m_mouse_button); }

//------------- MouseMovedEvent
    MouseMovedEvent::MouseMovedEvent(const double x, const double y) 
    : Basic_Event(EventType::MouseMoved), m_x_pos(x), m_y_pos(y) {}

    MouseMovedEvent::MouseMovedEvent(const std::pair<double, double>& pos) 
    : Basic_Event(EventType::MouseMoved), m_x_pos(pos.first), m_y_pos(pos.second) {}

    double MouseMovedEvent::GetX() const noexcept { return m_x_pos; }
    double MouseMovedEvent::GetY() const noexcept { return m_y_pos; }

    std::string MouseMovedEvent::ToString()   const noexcept { return std::format("MouseMovedEvent: pos: {}, {}", m_x_pos, m_y_pos); }

//------------- MouseScrolledEvent
    MouseScrolledEvent::MouseScrolledEvent(const double x_offset, const double y_offset) 
    : Basic_Event(EventType::MouseScrolled), m_x_offset(x_offset), m_y_offset(y_offset) {}

    MouseScrolledEvent::MouseScrolledEvent(const std::pair<double, double>& offset) 
    : Basic_Event(EventType::MouseScrolled), m_x_offset(offset.first), m_y_offset(offset.second) {}

    double MouseScrolledEvent::GetXOffset() const noexcept { return m_x_offset; }
    double MouseScrolledEvent::GetYOffset() const noexcept { return m_y_offset; }

    std::string MouseScrolledEvent::ToString() const noexcept { return std::format("MouseScrolledEvent: xOffset: {}, yOffset: {}", m_x_offset, m_y_offset); }

//------------- KeyPressedEvent
    KeyPressedEvent::KeyPressedEvent(const int key) 
    : Basic_Event(EventType::KeyPressed), m_key(key) {}

    int KeyPressedEvent::GetKeyType() const noexcept { return m_key; }

    std::string KeyPressedEvent::ToString() const noexcept  { return std::format("KeyPressedEvent: {}", m_key); }

//------------- KeyReleasedEvent
    KeyReleasedEvent::KeyReleasedEvent(const int key) 
    : Basic_Event(EventType::KeyReleased), m_key(key) {}

    int KeyReleasedEvent::GetKeyType() const noexcept { return m_key; }

    std::string KeyReleasedEvent::ToString() const noexcept { return std::format("KeyReleasedEvent: {}", m_key); }
}