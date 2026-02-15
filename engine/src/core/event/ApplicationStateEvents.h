#pragma once

#include "core/event/Event.h"

namespace CoreEngine
{
    class ApplicationShutdownEvent : public Basic_Event
    {
        public:
            explicit ApplicationShutdownEvent() noexcept;

            [[nodiscard]] constexpr static inline EventType GetStaticType() noexcept { return EventType::ApplicationShutdown; }
            [[nodiscard]] virtual std::string ToString() const noexcept override;
    };
}