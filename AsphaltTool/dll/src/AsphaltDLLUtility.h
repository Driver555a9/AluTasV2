#pragma once

#include <cstdint>
#include <iomanip>
#include <fstream>
#include <string>
#include <unordered_map>
#include <string_view>
#include <iostream>
#include <optional>
#include <vector>
#include <mutex>
#include <sstream>
#include <format>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "BulletTypes.h"

namespace AsphaltDLL
{
    namespace Utility
    {
        void InitConsole() noexcept;
        void ShutdownConsole() noexcept;
        void ClearConsole() noexcept;

        bool InitDebugLog(const std::filesystem::path& path) noexcept;
        void ShutdownDebugLog() noexcept;
        void LogToFile(const std::string& str) noexcept;
        void LogToFile(const char* str) noexcept;

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

        template <typename T, typename... Args>
        requires (std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Args>> && ...)
        constexpr bool EqualsAny(T&& val, Args&&... args)
        {
            return ((val == args) || ...);
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

        template <typename TVal>
        class DebugValCompare
        {
        public:
            explicit DebugValCompare(const std::string& log_file_path) noexcept : m_log_file_path(log_file_path)
            {
                m_log_file.open(m_log_file_path, std::ios::out | std::ios::trunc);
                if (m_log_file.is_open())
                {
                    m_log_file << "--- DebugValCompare Session Started ---\n";
                    m_log_file.flush();
                }
            }

            ~DebugValCompare()
            {
                if (m_log_file.is_open())
                {
                    m_log_file << "--- DebugValCompare Session Ended ---\n";
                    m_log_file.close();
                }
            }

            DebugValCompare(const DebugValCompare&) = delete;
            DebugValCompare& operator=(const DebugValCompare&) = delete;

            void AddValue(uint32_t tick, TVal val) noexcept
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto it = m_value_at_tick.find(tick);
                if (it == m_value_at_tick.end())
                {
                    m_log_file << "[ADDED]    Tick " << tick << " | Rec: " << val << "\n";
                    m_value_at_tick[tick] = val;
                }
                else
                {
                    const TVal& recorded_val = it->second;

                    if (m_log_file.is_open())
                    {
                        if constexpr (std::is_floating_point_v<TVal>)
                        {
                            m_log_file << std::setprecision(8) << std::fixed;
                        }

                        if (recorded_val == val)
                        {
                            m_log_file << "[MATCH]    Tick " << tick << " | Rec: " << recorded_val << " == Cur: " << val << "\n";
                        }
                        else
                        {
                            m_log_file << "[MISMATCH] Tick " << tick << " | Rec: " << recorded_val << " != Cur: " << val << "\n";
                        }
                        m_log_file.flush();
                    }
                }
            }

            void ClearRecordedData() noexcept
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_value_at_tick.clear();
                if (m_log_file.is_open())
                {
                    m_log_file << "--- Data Cleared ---\n";
                    m_log_file.flush();
                }
            }

        private:
            std::string m_log_file_path;
            std::ofstream m_log_file;
            std::unordered_map<uint32_t, TVal> m_value_at_tick;
            std::mutex m_mutex;
        };

        template <typename TVal>
        class DebugValForce
        {
        public:
            explicit DebugValForce() noexcept = default;
            ~DebugValForce() = default;

            DebugValForce(const DebugValForce&) = delete;
            DebugValForce& operator=(const DebugValForce&) = delete;

            void ForceValue(uint32_t tick, TVal& val) noexcept
            {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto it = m_value_at_tick.find(tick);
                if (it == m_value_at_tick.end())
                {
                    m_value_at_tick[tick] = val;
                }
                else
                {
                    val = it->second;
                }
            }

            void ClearRecordedData() noexcept
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_value_at_tick.clear();
            }

        private:
            std::unordered_map<uint32_t, TVal> m_value_at_tick;
            std::mutex m_mutex;
        };
    }
}

#if defined(_MSC_VER)
    consteval const char* GetFileName(const char* path) 
    {
        const char* file = path;
        for (const char* p = path; *p; ++p) 
        {
            if (*p == '/' || *p == '\\') 
            {
                file = p + 1;
            }
        }
        return file;
    }

    #define __FILENAME__HELPER__ GetFileName(__FILE__)

    #define DLL_ERROR_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILENAME__HELPER__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_LOG_FILE_NO_TEXT() std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILENAME__HELPER__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ::AsphaltDLL::Utility::ColorCodes::RESET << std::endl

    #define DLL_INFO_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::YELLOW \
    << "\n[INFO] File: " << __FILENAME__HELPER__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_LOG_FILE(expr) \
        do { \
            std::ostringstream _log_stream; \
            _log_stream << "[ERROR]: File: " << __FILENAME__HELPER__ << " Line: " << __LINE__ << ": " << expr; \
            ::AsphaltDLL::Utility::LogToFile(_log_stream.str()); \
        } while (false)

    #define DLL_INFO_LOG_FILE(expr) \
        do { \
            std::ostringstream _log_stream; \
            _log_stream << "[INFO]: File: " << __FILENAME__HELPER__ << " Line: " << __LINE__ << ": " << expr; \
            ::AsphaltDLL::Utility::LogToFile(_log_stream.str()); \
        } while (false)

    #define DLL_ERROR_LOG_FILE_FORMATED(expr) \
        do { \
            constexpr const char* file = __FILENAME__HELPER__; \
            ::AsphaltDLL::Utility::LogToFile(std::format("[ERROR]: File: {} Line: {}: {}", file, __LINE__, std::format(expr))); \
        } while (false)

    #define DLL_INFO_LOG_FILE_FORMATED(expr) \
        do { \
            constexpr const char* file = __FILENAME__HELPER__; \
            ::AsphaltDLL::Utility::LogToFile(std::format("[INFO]: File: {} Line: {}: {}", file, __LINE__, std::format(expr))); \
        } while (false)

#elif defined(__GNUC__)

    #define DLL_ERROR_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_LOG_FILE_NO_TEXT() std::cout << ::AsphaltDLL::Utility::ColorCodes::RED \
    << "\n[ERROR] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ::AsphaltDLL::Utility::ColorCodes::RESET << std::endl

    #define DLL_INFO_PRINT(expr) std::cout << ::AsphaltDLL::Utility::ColorCodes::YELLOW \
    << "\n[INFO] File: " << __FILE_NAME__ << ::AsphaltDLL::Utility::ColorCodes::GREEN \
    << " Line " << __LINE__ << ": " << ::AsphaltDLL::Utility::ColorCodes::RESET << expr << std::endl

    #define DLL_ERROR_LOG_FILE(expr) \
        do { \
            std::ostringstream _log_stream; \
            _log_stream << "[ERROR]: File: " << __FILE_NAME__ << " Line: " << __LINE__ << ": " << expr; \
            ::AsphaltDLL::Utility::LogToFile(_log_stream.str()); \
        } while (false)

    #define DLL_INFO_LOG_FILE(expr) \
        do { \
            std::ostringstream _log_stream; \
            _log_stream << "[INFO]: File: " << __FILE_NAME__ << " Line: " << __LINE__ << ": " << expr; \
            ::AsphaltDLL::Utility::LogToFile(_log_stream.str()); \
        } while (false)

    #define DLL_ERROR_LOG_FILE_FORMATED(expr) \
        do { \
            constexpr const char* file = __FILE_NAME__; \
            ::AsphaltDLL::Utility::LogToFile(std::format("[ERROR]: File: {} Line: {}: {}", file, __LINE__, std::format(expr))); \
        } while (false)

    #define DLL_INFO_LOG_FILE_FORMATED(expr) \
        do { \
            constexpr const char* file = __FILE_NAME__; \
            ::AsphaltDLL::Utility::LogToFile(std::format("[INFO]: File: {} Line: {}: {}", file, __LINE__, std::format(expr))); \
        } while (false)
#endif 