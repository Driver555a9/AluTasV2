#include "core/utility/ColorCodes.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif 

[[nodiscard]] static bool _CONSOLE_SUPPORTS_VT() noexcept
{
#ifdef _WIN32
    static bool checked = false;
    static bool supported = false;

    if (!checked) 
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (hOut && GetConsoleMode(hOut, &mode)) 
        {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            supported = SetConsoleMode(hOut, mode) != 0;
        }
        checked = true;
    }
    return supported;
#else
    return true;
#endif
}

namespace CoreEngine
{ 
    namespace ColorCodes
    {
        std::string_view GetColor(std::string_view code) noexcept
        {
            return _CONSOLE_SUPPORTS_VT() ? code : "";
        }
    }
}