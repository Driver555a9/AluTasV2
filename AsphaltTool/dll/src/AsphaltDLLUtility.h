#pragma once

#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string_view>
#include <iostream>
#include <vector>
#include <array>

namespace AsphaltDLL
{
    namespace Utility
    {
        void InitConsole() noexcept;
        void ShutdownConsole() noexcept;
        void ClearConsole() noexcept;

        [[nodiscard]] float RandomFloat(float min, float max) noexcept;
        [[nodiscard]] int RandomInt(int min, int max) noexcept;
        
        [[nodiscard]] uintptr_t SafeResolvePointerChain(uintptr_t module_base, const std::vector<uintptr_t>& offsets) noexcept;
        std::string LPCWSTRToString(LPCWSTR lpcwstr) noexcept;

        struct QuaternionXZYW 
        {
            float x;
            float z;
            float y;
            float w;
        };
        [[nodiscard]] QuaternionXZYW RotationExtractQuatCast(const std::array<float, 9>& mat) noexcept;
        [[nodiscard]] std::array<float, 3> RotateVectorByQuaternionXZYW(const Utility::QuaternionXZYW& q, const std::array<float, 3>& v) noexcept;
                
        namespace ColorCodes
        {
            constexpr std::string_view  RESET   = "\033[0m";
            constexpr std::string_view  RED     = "\033[31m";
            constexpr std::string_view  GREEN   = "\033[32m";
            constexpr std::string_view  YELLOW  = "\033[33m";
            constexpr std::string_view  BLUE    = "\033[34m";
            constexpr std::string_view  WHITE   = "\033[37m";
        }
    }
}

#if defined(_MSC_VER)
    #define ___FILENAME_HELPER_MACRO___ (std::strrchr(__FILE__, '/') ? std::strrchr(__FILE__, '/') + 1 : std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

    #define DLL_ERROR_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << ___FILENAME_HELPER_MACRO___ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_PRINT_NO_TEXT() std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << ___FILENAME_HELPER_MACRO___ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ::AsphaltDLL::Utility::ColorCodes::RESET << std::endl

    #define DLL_INFO_LOG(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::YELLOW \
    << "\n[INFO] File: " << ___FILENAME_HELPER_MACRO___ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl
#elif defined(__GNUC__)

    #define DLL_ERROR_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_PRINT_NO_TEXT() std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ::AsphaltDLL::Utility::ColorCodes::RESET << std::endl

    #define DLL_INFO_LOG(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::YELLOW \
    << "\n[INFO] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl
#endif 