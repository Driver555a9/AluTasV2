#pragma once

#include "core/event/Event.h"

namespace CoreEngine
{
    class MousePressedEvent : public Basic_Event 
    {
        protected:
            const int m_mouse_button;
            const double m_x_pos, m_y_pos;

        public:
            explicit MousePressedEvent(const int button, const double x, const double y);
            explicit MousePressedEvent(const int button, const std::pair<double, double>& pos);

            [[nodiscard]] double GetX() const noexcept;
            [[nodiscard]] double GetY() const noexcept;
            [[nodiscard]] int GetMouseButton() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::MousePressed; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class MouseReleasedEvent : public Basic_Event 
    {
        protected:
            const int m_mouse_button;
            const double m_x_pos, m_y_pos;

        public:
            explicit MouseReleasedEvent(const int button, const double x, const double y);
            explicit MouseReleasedEvent(const int button, const std::pair<double, double>& pos);

            [[nodiscard]] double GetX() const noexcept;
            [[nodiscard]] double GetY() const noexcept;
            [[nodiscard]] int GetMouseButton() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::MouseReleased; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class MouseMovedEvent : public Basic_Event 
    {
        protected:
            const double m_x_pos, m_y_pos;

        public:
            explicit MouseMovedEvent(const double x, const double y);
            explicit MouseMovedEvent(const std::pair<double, double>& pos);

            [[nodiscard]] double GetX() const noexcept;
            [[nodiscard]] double GetY() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::MouseMoved; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class MouseScrolledEvent : public Basic_Event
    {
        protected:
            const double m_x_offset, m_y_offset; 
        
        public:
            explicit MouseScrolledEvent(const double x_offset, const double y_offset);
            explicit MouseScrolledEvent(const std::pair<double, double>& offset);

            [[nodiscard]] double GetXOffset() const noexcept;
            [[nodiscard]] double GetYOffset() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::MouseScrolled; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class KeyPressedEvent : public Basic_Event
    {
        protected:
            const int m_key;
        
        public:
            explicit KeyPressedEvent(const int key);

            [[nodiscard]] int GetKeyType() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::KeyPressed; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };

    class KeyReleasedEvent : public Basic_Event
    {
        protected:
            const int m_key;
        
        public:
            explicit KeyReleasedEvent(const int key);

            [[nodiscard]] int GetKeyType() const noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::KeyReleased; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };
}