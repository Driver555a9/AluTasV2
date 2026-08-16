#pragma once

#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string_view>
#include <iostream>
#include <optional>
#include <vector>
#include <array>

#include "BulletTypes.h"

namespace AsphaltDLL
{
    namespace Utility
    {
        void InitConsole() noexcept;
        void ShutdownConsole() noexcept;
        void ClearConsole() noexcept;

        [[nodiscard]] float RandomFloat(float min, float max) noexcept;
        [[nodiscard]] int RandomInt(int min, int max) noexcept;
        [[nodiscard]] uint64_t GetMonotonicMicrosecondCount() noexcept;
        
        [[nodiscard]] uintptr_t SafeResolvePointerChain(uintptr_t module_base, const std::vector<uintptr_t>& offsets) noexcept;

        template <typename T>
        std::optional<T*> SafeReadPointer(T* const* ptr) noexcept
        {
            T* result = nullptr;
            __try
            {
                if (ptr) result = *ptr;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) 
            {
                return std::nullopt;
            }
            return result;
        }

        template <typename T>
        std::optional<T*> SafeDereference(T* ptr) noexcept
        {
            __try
            {
                if (!ptr) return std::nullopt;

                volatile char dummy = *reinterpret_cast<volatile char*>(ptr);
                (void)dummy;
                return ptr;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return std::nullopt;
            }
        }

        std::string LPCWSTRToString(LPCWSTR lpcwstr) noexcept;

        [[nodiscard]] BulletTypes::Quaternion RotationFromTransform(const BulletTypes::Transform& mat) noexcept;
        [[nodiscard]] BulletTypes::Vector3 RotateVectorByQuaternion(const BulletTypes::Quaternion& q, const BulletTypes::Vector3& v) noexcept;
                
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

    #define DLL_DEBUG_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::BLUE \
    << "\n[ERROR] File: " << ___FILENAME_HELPER_MACRO___ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
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

    #define DLL_DEBUG_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::BLUE \
    << "\n[INFO] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl
#endif 