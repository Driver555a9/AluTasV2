#include "tas/memory/MemoryUtility.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <tlhelp32.h>
#endif

#include <thread>
#include <atomic>

namespace AsphaltTas::MemoryUtility
{
//////////////////////////////////////////////////////////
// Get Asphalt process
//////////////////////////////////////////////////////////
    static std::optional<libmem::Process> g_GAME_PROCESS_OPT = std::nullopt;
    static std::optional<libmem::Module>  g_GAME_MODULE_OPT  = std::nullopt;
    static std::atomic<bool> g_HAS_CACHED_PROCESS = false;
    static std::atomic<bool> g_HAS_CACHED_MODULE  = false;

    libmem::Process GetAsphaltProcessOrThrow()
    {
        if (! GameState::GetHasValidCurrentPlatform())
            throw MemoryManipFailedException("MemoryUtility: Could not read because no platform has been set.");

        if (g_HAS_CACHED_PROCESS.load(std::memory_order::acquire))
        {
            return g_GAME_PROCESS_OPT.value();
        }

        const GameState::GamePlatform platform = GameState::GetCurrentPlatform();

        g_GAME_PROCESS_OPT = libmem::FindProcess(GameState::GetGameExeNameFromPlatform(platform));
        if (! g_GAME_PROCESS_OPT.has_value()) 
            throw MemoryManipFailedException("MemoryUtility: Failed to open process.");

        g_HAS_CACHED_PROCESS.store(true, std::memory_order::release);

        return g_GAME_PROCESS_OPT.value();
    }

    std::pair<libmem::Process, libmem::Module> GetAsphaltProcessAndModuleOrThrow()
    {
        if (! GameState::GetHasValidCurrentPlatform())
            throw MemoryManipFailedException("MemoryUtility: Could not read because no platform has been set.");

        if (g_HAS_CACHED_PROCESS.load(std::memory_order::acquire) && g_HAS_CACHED_MODULE.load(std::memory_order::acquire))
        {
            return { g_GAME_PROCESS_OPT.value(), g_GAME_MODULE_OPT.value() };
        }

        GameState::GamePlatform platform = GameState::GetCurrentPlatform();

        g_GAME_PROCESS_OPT = libmem::FindProcess(GameState::GetGameExeNameFromPlatform(platform));
        if (!g_GAME_PROCESS_OPT) throw MemoryManipFailedException("MemoryUtility: Failed to open process.");
        g_HAS_CACHED_PROCESS.store(true, std::memory_order::release);

        g_GAME_MODULE_OPT = libmem::FindModule(&g_GAME_PROCESS_OPT.value(), GameState::GetGameExeNameFromPlatform(platform));
        if (!g_GAME_MODULE_OPT) throw MemoryManipFailedException("MemoryUtility: Failed to open process.");
        g_HAS_CACHED_MODULE.store(true, std::memory_order::release);

        return { g_GAME_PROCESS_OPT.value(), g_GAME_MODULE_OPT.value() };
    }

//////////////////////////////////////////////////////////
// Pattern searching
//////////////////////////////////////////////////////////
    libmem::Address AOBScanOrThrow(const libmem::Process* process, const char* pattern, libmem::Address begin, size_t size)
    {
        std::optional<libmem::Address> opt_addr = libmem::SigScan(process, pattern, begin, size);
        if (! opt_addr) throw MemoryManipFailedException("MemoryUtility: Failed to find aob pattern.");
        return opt_addr.value();
    }

//////////////////////////////////////////////////////////
// Allocating
//////////////////////////////////////////////////////////
    libmem::Address AllocMemoryOrThrow(const libmem::Process* process, size_t size, libmem::Prot protection)
    {
        std::optional<libmem::Address> opt_addr = libmem::AllocMemory(process, size, protection);
        if (!opt_addr) throw MemoryManipFailedException("MemoryUtility: Failed to allocate memory.");
        return opt_addr.value();
    }

#ifdef _WIN32
    libmem::Address AllocMemoryNearAddressOrThrow(const libmem::Process* process, libmem::Address desired_address, size_t size, uintptr_t max_distance)
    {
        constexpr uintptr_t STEP     = 0x10000;
        constexpr uintptr_t MIN_ADDR = 0x10000;

        HANDLE handle_process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, process->pid);
        if (! handle_process)
            throw MemoryManipFailedException("MemoryUtility: Failed to OpenProcess for pid: " + std::to_string(process->pid));

        uintptr_t base     = desired_address;
        uintptr_t min_scan = (base > max_distance) ? base - max_distance : MIN_ADDR;
        uintptr_t max_scan = base + max_distance;
        min_scan &= ~0xFFFull;
        max_scan &= ~0xFFFull;

        for (uintptr_t delta = STEP; delta <= max_distance; delta += STEP)
        {
            uintptr_t candidate_lo = (delta <= base) ? base - delta : MIN_ADDR;
            if (candidate_lo >= min_scan && candidate_lo <= max_scan)
            {
                LPVOID result = VirtualAllocEx(handle_process, reinterpret_cast<LPVOID>(candidate_lo), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                if (result)
                {
                    CloseHandle(handle_process);
                    return reinterpret_cast<libmem::Address>(result);
                }
            }

            uintptr_t candidate_hi = base + delta;
            if (candidate_hi >= min_scan && candidate_hi <= max_scan)
            {
                LPVOID result = VirtualAllocEx(handle_process, reinterpret_cast<LPVOID>(candidate_hi), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                if (result)
                {
                    CloseHandle(handle_process);
                    return reinterpret_cast<libmem::Address>(result);
                }
            }
        }

        CloseHandle(handle_process);
        throw MemoryManipFailedException("MemoryUtility: Failed AllocMemoryNear. Base=" + std::to_string(base) + " size=" + std::to_string(size) +
            " max_distance=" + std::to_string(max_distance));
    }
#endif

//////////////////////////////////////////////////////////
// Freeing
//////////////////////////////////////////////////////////
    void FreeMemoryOrThrow(const libmem::Process* process, libmem::Address address, size_t size)
    {
        if (! TryFreeMemoryOrNothing(process, address, size))
            throw std::runtime_error("Failed to free memory.");
    }

    bool TryFreeMemoryOrNothing(const libmem::Process* process, libmem::Address address, size_t size) noexcept
    {
        bool result = libmem::FreeMemory(process, address, size);
        if (! result) ENGINE_ERROR_PRINT("Warning: Failed to free memory at address: " << address << " with size: " << size);
        return result;
    }

#ifdef _WIN32    
    void FreeMemoryAllocatedNearAddressOrThrow(const libmem::Process* process, libmem::Address address, size_t size)
    {
        HANDLE handle_process = OpenProcess(PROCESS_VM_OPERATION, FALSE, process->pid);
        if (handle_process)
        {
            VirtualFreeEx(handle_process, reinterpret_cast<LPVOID>(address), size, MEM_RELEASE);
            CloseHandle(handle_process);
        } else 
        {
            throw MemoryManipFailedException("MemoryUtility: Failed to free memory allocated near address, at address: " + std::to_string(address));
        }
    }
#endif
    
//////////////////////////////////////////////////////////
// Reading
//////////////////////////////////////////////////////////
    void ReadMemoryOrThrow(const libmem::Process* process, libmem::Address address, void* begin, size_t size)
    {
        if (! TryReadMemoryOrNothing(process, address, reinterpret_cast<uint8_t*>(begin), size))
            throw MemoryManipFailedException("MemoryUtility: Failed to read memory.");
    }

    bool TryReadMemoryOrNothing(const libmem::Process* process, libmem::Address address, void* begin, size_t size) noexcept
    {
        return libmem::ReadMemory(process, address, reinterpret_cast<uint8_t*>(begin), size) == size;
    }

    void ReadFloatOrThrow(const libmem::Process* process, libmem::Address address, float& out)
    {
        ReadMemoryOrThrow(process, address, reinterpret_cast<uint8_t*>(&out), sizeof(decltype(out)));
    }

//////////////////////////////////////////////////////////
// Writting
//////////////////////////////////////////////////////////
    void WriteMemoryOrThrow(const libmem::Process* process,  libmem::Address address, void* begin, size_t size)
    {
        if (libmem::WriteMemory(process, address, reinterpret_cast<uint8_t*>(begin), size) != size)
            throw MemoryManipFailedException("MemoryUtility: Failed to write memory.");
    }

    bool TryWriteMemoryOrNothing(const libmem::Process* process,  libmem::Address address, void* begin, size_t size) noexcept
    {
        return libmem::WriteMemory(process, address, reinterpret_cast<uint8_t*>(begin), size) == size;
    }

    void WriteFloatOrThrow(const libmem::Process* process, libmem::Address address, float data)
    {
        WriteMemoryOrThrow(process, address, &data, sizeof(decltype(data)));
    }

//////////////////////////////////////////////////////////
// Suspend / Pause process
//////////////////////////////////////////////////////////
#ifdef WIN32
    SuspendedProcess::~SuspendedProcess() noexcept
    {
        Resume();
    }
    
    void SuspendedProcess::Resume() noexcept
    {
        for (HANDLE handle : m_thread_handles)
        {
            ResumeThread(handle);
            CloseHandle(handle);
        }
        m_thread_handles.clear();
    }

    std::optional<SuspendedProcess> SuspendProcess(libmem::Pid process_id) noexcept
    {
        SuspendedProcess suspended;
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

        if (snapshot == INVALID_HANDLE_VALUE)
        {
            ENGINE_ERROR_PRINT("Failed to create thread snapshot");
            return std::nullopt;
        }

        THREADENTRY32 thread_entry;
        thread_entry.dwSize = sizeof(THREADENTRY32);

        if (! Thread32First(snapshot, &thread_entry))
        {
            CloseHandle(snapshot);
            ENGINE_ERROR_PRINT("Failed to enumerate threads");
            return std::nullopt;
        }

        do
        {
            if (thread_entry.th32OwnerProcessID == process_id)
            {
                HANDLE thread_handle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, thread_entry.th32ThreadID);
                if (thread_handle != nullptr)
                {
                    if (SuspendThread(thread_handle) != static_cast<DWORD>(-1))
                    {
                        suspended.m_thread_handles.push_back(thread_handle);
                    }
                    else
                    {
                        CloseHandle(thread_handle);
                        ENGINE_ERROR_PRINT("Failed to suspend thread " << thread_entry.th32ThreadID);
                    }
                }
            }
        } while (Thread32Next(snapshot, &thread_entry));

        CloseHandle(snapshot);
        
        if (suspended.m_thread_handles.empty())
        {
            ENGINE_ERROR_PRINT("No threads were suspended");
            return std::nullopt;
        }

        return suspended;
    }

//////////////////////////////////////////////////////////
// Process visibility state (Windows API required)
//////////////////////////////////////////////////////////

    bool ProcessIsInForeground(libmem::Pid process_id) noexcept
    {
        HWND fg = GetForegroundWindow();
        DWORD pid;
        GetWindowThreadProcessId(fg, &pid);

        return pid == static_cast<DWORD>(process_id);
    }


//////////////////////////////////////////////////////////
// Get HWND from pid
//////////////////////////////////////////////////////////

    static HWND g_FOUND_HWND = nullptr;

    static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM pid_to_find) noexcept
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)pid_to_find && IsWindowVisible(hwnd))
        {
            g_FOUND_HWND = hwnd;
            return FALSE;
        }
        return TRUE;
    }

    HWND GetHWNDFromPID(libmem::Pid pid) noexcept
    {
    
        g_FOUND_HWND = nullptr;
        EnumWindows(EnumWindowsCallback, (LPARAM)pid);
        return g_FOUND_HWND;
    }

//////////////////////////////////////////////////////////
// Call callback once external application closes
//////////////////////////////////////////////////////////

    void LaunchApplicationShutdownWatchdogThread(libmem::Pid pid, const std::function<void()>& callback) noexcept
    {
        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);

        if (!hProc) return;

        std::thread([hProc, callback]() 
        { 
            WaitForSingleObject(hProc, INFINITE); 
            CloseHandle(hProc);
            callback();
        }).detach();
    }

    
//////////////////////////////////////////////////////////
// Fetch window position + width & height
//////////////////////////////////////////////////////////
    WindowInfo GetWindowInfoOrThrow(HWND hwnd)
    {
        RECT client;
        if (!GetClientRect(hwnd, &client))
            throw std::runtime_error("GetClientRect failed");

        POINT tl { client.left, client.top };
        POINT br { client.right, client.bottom };

        if (!ClientToScreen(hwnd, &tl) || !ClientToScreen(hwnd, &br))
            throw std::runtime_error("ClientToScreen failed");

        return WindowInfo{
            .m_x_position = tl.x,
            .m_y_position = tl.y,
            .m_width      = br.x - tl.x,
            .m_height     = br.y - tl.y
        };
    }

#endif

//////////////////////////////////////////////////////////
// Invalidate all cache
//////////////////////////////////////////////////////////
    void InvalidateCache() noexcept
    {
        g_HAS_CACHED_PROCESS.store(false, std::memory_order::release);
        g_HAS_CACHED_MODULE.store(false, std::memory_order::release);
    }
}
