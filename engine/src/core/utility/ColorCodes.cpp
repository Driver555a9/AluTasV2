#include "core/utility/ColorCodes.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif 

static bool _CONSOLE_SUPPORTS_VT() noexcept
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut && GetConsoleMode(hOut, &mode))
        return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    return false;
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
            //return _CONSOLE_SUPPORTS_VT() ? code : "";
            return code;
        }
    }
}