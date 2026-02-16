#pragma once

#include <string>

#include "core/utility/Units.h"

namespace CoreEngine
{
    struct GlobalGet_FramebufferSize     { using type = std::pair<int, int>; };
    struct GlobalGet_AspectRatio         { using type = float;               };
    struct GlobalGet_VsyncIsOn           { using type = bool;                };
    struct GlobalGet_ApplicationName     { using type = std::string;         };
    struct GlobalGet_DeltaTimeMicros     { using type = Units::MicroSecond;  };
    struct GlobalGet_DeltaTimeSeconds    { using type = Units::Second;       };

    template <typename TGet>
    auto GlobalGet() -> TGet::type;
    
//-------------------
    struct GlobalSet_VsyncIsOn {};
    struct GlobalSet_StopApplication {};
    struct GlobalSet_RaiseEvent {};

    template <typename TSet, typename Arg>
    void GlobalSet(Arg&&);

    template <typename TSet>
    void GlobalSet();
}