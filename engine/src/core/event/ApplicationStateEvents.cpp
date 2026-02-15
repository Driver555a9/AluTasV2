#include "core/event/ApplicationStateEvents.h"

//std
#include <format>

namespace CoreEngine
{
//------------- ApplicationShutdownEvent
    ApplicationShutdownEvent::ApplicationShutdownEvent() noexcept : Basic_Event(EventType::ApplicationShutdown) {}

    std::string ApplicationShutdownEvent::ToString()   const noexcept { return std::format("ApplicationShutdownEvent"); }
}