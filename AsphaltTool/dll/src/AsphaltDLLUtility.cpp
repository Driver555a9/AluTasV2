#include "AsphaltDLLUtility.h"
#include <cstdint>
#include <cstdio>
#include <random>

namespace AsphaltDLL::Utility
{
    void InitConsole() noexcept 
    {
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);

        HANDLE handles[2] = { GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE) };

        for (HANDLE h : handles)
        {
            DWORD mode = 0;
            if (h && GetConsoleMode(h, &mode))
            {
                mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h, mode);
            }
        }
    }

    void ShutdownConsole() noexcept
    {
        fclose(stdout);
        fclose(stderr);
        FreeConsole();
    }

    void ClearConsole() noexcept
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = {0, 0};
        DWORD count;
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        GetConsoleScreenBufferInfo(hConsole, &csbi);
        DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;

        FillConsoleOutputCharacter(hConsole, ' ', cellCount, coord, &count);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, coord, &count);
        SetConsoleCursorPosition(hConsole, coord);
    }

    float RandomFloat(float min, float max) noexcept
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }
        
    int RandomInt(int min, int max) noexcept
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }

    uintptr_t SafeResolvePointerChain(uintptr_t module_base, const std::vector<uintptr_t>& offsets) noexcept
    {
        __try 
        {
            uintptr_t out = module_base;
            for (const auto it : offsets)
            {   
                out = *reinterpret_cast<uintptr_t*>(out + it);
            }
            return out;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            return 0;
        }
    }

    std::string LPCWSTRToString(LPCWSTR lpcwstr) noexcept
    {
        if (!lpcwstr) return "";

        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, NULL, 0, NULL, NULL);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, lpcwstr, -1, &str[0], sizeNeeded, NULL, NULL);

        return str;
    }
}