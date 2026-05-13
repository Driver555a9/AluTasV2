#pragma once

#ifdef _WIN32

#include "Communication.h"

#include <optional>
#include <mutex>

namespace AsphaltTas
{
    template <typename T>
    class ScopeLockedAccess 
    {
    public:
        ScopeLockedAccess(std::mutex& m, T& v) : m_lock(m), m_value(v) {}
        T& Get() { return m_value; }
        T* operator->() { return &m_value; }
        T& operator*() { return m_value; }

        ScopeLockedAccess(const ScopeLockedAccess&)            = delete;
        ScopeLockedAccess& operator=(const ScopeLockedAccess&) = delete;
        ScopeLockedAccess(ScopeLockedAccess&&)                 = delete;
        ScopeLockedAccess& operator=(ScopeLockedAccess&&)      = delete;

    private:
        std::scoped_lock<std::mutex> m_lock;
        T& m_value;
    };

    ///////////////////////////////////////////////////
    // Multithread policy:
    // Access to shared state ring buffers only within AsphaltDllManager or functions that it exclusively calls
    // Access to in/out states exclusively with forced mutex scoped locks
    ///////////////////////////////////////////////////

    namespace AsphaltDllManager
    {  
        void UpdateCurrentCommunicationState() noexcept;

        ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>> GetDllStateOutLockResultRef() noexcept;
        ScopeLockedAccess<ComDllIn::DllReplayInputIn> GetDllReplayInputInRef() noexcept;
        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> GetDllGeneralCommandsInRef() noexcept;

        std::optional<ComDllOut::DllStateOut> GetDllStateOutCopy() noexcept;
        ComDllIn::DllReplayInputIn GetDllReplayInputsInCopy() noexcept;
        ComDllIn::DllGeneralCommandsIn GetDllGeneralCommandsInCopy() noexcept;

        void InjectIntoGame();
        void EjectFromGame();
        [[nodiscard]] bool IsInjected() noexcept;

        constexpr char g_dll_name_char[]       = "AsphaltToolDLL.dll";
        constexpr wchar_t g_dll_name_wchar_t[] = L"AsphaltToolDLL.dll";
    }
}

#endif