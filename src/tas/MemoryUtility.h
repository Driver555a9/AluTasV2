#pragma once

#include "libmem/libmem.hpp"

#include "tas/GameState.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <functional>

#ifdef _WIN32
    struct HWND__;
    using HWND = HWND__*;
#endif

namespace AsphaltTas
{
    namespace MemoryUtility
    {
    //////////////////////////////////////////////////////////
    // Exceptions thrown by functions
    //////////////////////////////////////////////////////////
        struct MemoryManipFailedException : public std::runtime_error { explicit MemoryManipFailedException(const char* what) noexcept : std::runtime_error(what) {} };

    //////////////////////////////////////////////////////////
    // Get process
    //////////////////////////////////////////////////////////
        [[nodiscard]] libmem::Process GetAsphaltProcessOrThrow();

        [[nodiscard]] std::pair<libmem::Process, libmem::Module> GetAsphaltProcessAndModuleOrThrow();

    //////////////////////////////////////////////////////////
    // Pattern searching
    //////////////////////////////////////////////////////////
        [[nodiscard]] libmem::Address AOBScanOrThrow(const libmem::Process* process, const char* pattern, libmem::Address begin, size_t size);

    //////////////////////////////////////////////////////////
    // Allocating
    //////////////////////////////////////////////////////////
        [[nodiscard]] libmem::Address AllocMemoryOrThrow(const libmem::Process* process, size_t size, libmem::Prot protection);

    //////////////////////////////////////////////////////////
    // Freeing
    //////////////////////////////////////////////////////////
        void FreeMemoryOrThrow(const libmem::Process* process, libmem::Address address, size_t size);

        bool TryFreeMemoryOrNothing(const libmem::Process* process, libmem::Address address, size_t size) noexcept;

    //////////////////////////////////////////////////////////
    // Reading
    //////////////////////////////////////////////////////////
        void ReadMemoryOrThrow(const libmem::Process* process, libmem::Address address, void* begin, size_t size);

        bool TryReadMemoryOrNothing(const libmem::Process* process, libmem::Address address, void* begin, size_t size) noexcept;

        void ReadFloatOrThrow(const libmem::Process* process, libmem::Address address, float& out);

        template <typename T>
        inline void ReadGlmValueOrThrow(const libmem::Process* process, libmem::Address address, T& out)
        {
            ReadMemoryOrThrow(process, address, reinterpret_cast<uint8_t*>(glm::value_ptr(out)), sizeof(decltype(out)));
        }

    //////////////////////////////////////////////////////////
    // Writing
    //////////////////////////////////////////////////////////
        void WriteMemoryOrThrow(const libmem::Process* process, libmem::Address address, void* begin, size_t size);

        bool TryWriteMemoryOrNothing(const libmem::Process* process,  libmem::Address address, void* begin, size_t size) noexcept;

        void WriteFloatOrThrow(const libmem::Process* process, libmem::Address address, float data);

        template <typename T>
        inline void WriteGlmValueOrThrow(const libmem::Process* process, libmem::Address address, T&& data)
        {
            WriteMemoryOrThrow(process, address, reinterpret_cast<uint8_t*>(glm::value_ptr(data)), sizeof(decltype(data)));
        }

#ifdef _WIN32
    //////////////////////////////////////////////////////////
    // Suspend / Pause process (Windows API required)
    //////////////////////////////////////////////////////////
        struct SuspendedProcess
        {
            std::vector<void*> m_thread_handles;
            
            ~SuspendedProcess() noexcept;
            void Resume() noexcept;
        };

        [[nodiscard]] std::optional<SuspendedProcess> SuspendProcess(libmem::Pid process_id) noexcept;

    //////////////////////////////////////////////////////////
    // Process visibility state (Windows API required)
    //////////////////////////////////////////////////////////
        [[nodiscard]] bool ProcessIsInForeground(libmem::Pid process_id) noexcept;

    //////////////////////////////////////////////////////////
    // Get HWND from pid (Windows API required)
    //////////////////////////////////////////////////////////    
        [[nodiscard]] HWND GetHWNDFromPID(libmem::Pid process_id) noexcept;

    //////////////////////////////////////////////////////////
    // Call callback once external application closes (Windows API required)
    //////////////////////////////////////////////////////////
        void ApplicationShutdownWatchdog(libmem::Pid process_id, const std::function<void()>& callback) noexcept;
;
#endif

    //////////////////////////////////////////////////////////
    // Invalidate any cache
    //////////////////////////////////////////////////////////
        void InvalidateCache() noexcept;
    }

}