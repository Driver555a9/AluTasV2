#pragma once

#include "tas/common/Replay.h"
#include <filesystem>

namespace AsphaltTas
{
    namespace ReplaySerializer
    {
        void SaveBinary(const Replay& replay, const std::string& path) noexcept;
        [[nodiscard]] Replay LoadBinary(const std::string& path) noexcept;
    }
}