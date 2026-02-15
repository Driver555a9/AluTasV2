#pragma once

#include <iostream>
#include <string_view>

namespace CoreEngine
{
    namespace ColorCodes
    {
        constexpr std::string_view  RESET   = "\033[0m";
        constexpr std::string_view  RED     = "\033[31m";
        constexpr std::string_view  GREEN   = "\033[32m";
        constexpr std::string_view  YELLOW  = "\033[33m";
        constexpr std::string_view  BLUE    = "\033[34m";
        constexpr std::string_view  WHITE   = "\033[37m";

        [[nodiscard]] std::string_view GetColor(std::string_view code) noexcept;
    }
}
