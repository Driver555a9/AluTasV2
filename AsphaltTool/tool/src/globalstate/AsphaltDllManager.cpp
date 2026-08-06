#include "globalstate/AsphaltDllManager.h"

#include "AsphaltDllManager.h"
#include "Communication.h"
#include "core/utility/Assert.h"
#include "layer/TasInputLayer.h"
#include "memory/MemoryUtility.h"
#include "common/Utility.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <filesystem>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <tlhelp32.h>
#endif

namespace AsphaltTas
{
    namespace AsphaltDllManager
    {
    namespace  
    {
        std::atomic<bool> g_is_injected = false;

        inline std::optional<ComDllOut::DllStateOut> g_dll_state_out;
        inline std::mutex g_dll_state_out_mutex;

        inline ComDllIn::DllReplayInputIn g_dll_replay_inputs_command;
        inline std::mutex g_dll_replay_inputs_command_mutex;

        inline ComDllIn::DllGeneralCommandsIn g_dll_general_command;
        inline std::mutex g_dll_general_command_mutex;

        std::wstring g_dll_path_w_string = L"";
        std::string g_dll_path_string = "";

        std::string g_game_directory_path = "";

        [[nodiscard]] inline std::string WideStringToString(const std::wstring& wide) noexcept
        {
        #ifdef _WIN32
            if (wide.empty()) return {};

            int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);

            std::string utf8(size - 1, '\0');

            WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1,utf8.data(), size, nullptr,nullptr);

            return utf8;
        #endif 
            return "";
        }

        std::wstring ResolveFullDLLPath()
        {
            wchar_t main_exe_path[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, main_exe_path, MAX_PATH);

            if (len == 0 || len == MAX_PATH)
                throw std::runtime_error("ResolveFullDLLPath(): GetModuleFileNameW failed");

            std::wstring fullPath(main_exe_path);

            size_t pos = fullPath.find_last_of(L"\\/");
            if (pos == std::wstring::npos)
                throw std::runtime_error("ResolveFullDLLPath(): Could not extract directory");

            std::wstring dir = fullPath.substr(0, pos + 1);

            std::wstring dllName;
            dllName.assign(g_dll_name_char, g_dll_name_char + strlen(g_dll_name_char));

            return dir + dllName;
        }

        std::wstring GetProcessDirectory(HANDLE process)
        {
            DWORD size = 32768;
            std::wstring exePath(size, L'\0');

            if (!QueryFullProcessImageNameW(process, 0, exePath.data(), &size))
                throw std::runtime_error("QueryFullProcessImageNameW failed");

            exePath.resize(size);

            return std::filesystem::path(exePath).parent_path().wstring();
        }
    }

        void UpdateCurrentCommunicationState() noexcept
        {
            ComSharedMem::SharedState* shared = ComSharedMem::GetSharedState();
            ComDllOut::DllStateOut out_state;

            while (shared->m_dll_out_buffer.TryPop(out_state))
            {
                const bool is_in_race          = out_state.m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE;
                bool new_race_began            = false;
                bool race_ended                = false;
                const uint32_t current_tick    = out_state.m_replay_inputs.m_race_frame_tick;

                {
                    ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>> opt_prev_out_state = GetDllStateOutLockResultRef();
                    if (opt_prev_out_state->has_value())
                    {
                        new_race_began   =   is_in_race && (*opt_prev_out_state)->m_meta_data.m_race_status_state != ComDllOut::RaceStatusState::IN_RACE;
                        race_ended       = ! is_in_race && (*opt_prev_out_state)->m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE;
                    }

                    *opt_prev_out_state = out_state;
                }

                if (new_race_began)
                {
                    TasInputLayer::OnRaceStarted();
                }
                else if (race_ended)
                {
                    TasInputLayer::OnRaceEnded();
                }
                
                TasInputLayer::OnDLLUpdate();
            }

            ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd = GetDllGeneralCommandsInRef();
            general_cmd->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;

            shared->m_dll_in_buffer_general.PushOverwrite(*general_cmd);
        }

        ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>> GetDllStateOutLockResultRef() noexcept
        {
            return ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>>(g_dll_state_out_mutex, g_dll_state_out);
        }

        ScopeLockedAccess<ComDllIn::DllReplayInputIn> GetDllReplayInputInRef() noexcept
        {
            return ScopeLockedAccess<ComDllIn::DllReplayInputIn>(g_dll_replay_inputs_command_mutex, g_dll_replay_inputs_command);
        }

        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> GetDllGeneralCommandsInRef() noexcept
        {
            return ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn>(g_dll_general_command_mutex, g_dll_general_command);
        }

        std::optional<ComDllOut::DllStateOut> GetDllStateOutCopy() noexcept
        {
            std::scoped_lock<std::mutex> lock(g_dll_state_out_mutex);
            return g_dll_state_out;
        }

        ComDllIn::DllReplayInputIn GetDllReplayInputsInCopy() noexcept
        {
            std::scoped_lock<std::mutex> lock(g_dll_replay_inputs_command_mutex);
            return g_dll_replay_inputs_command;
        }

        ComDllIn::DllGeneralCommandsIn GetDllGeneralCommandsInCopy() noexcept
        {
            std::scoped_lock<std::mutex> lock(g_dll_general_command_mutex);
            return g_dll_general_command;
        }

        void InjectIntoGame()
        {
            ComSharedMem::SharedState* shared = ComSharedMem::GetSharedState();
            shared->m_non_negotiable_communication_version = Communication::CURRENT_NON_NEGOTIABLE_COMMUNICATION_VERSION;
            
            if (IsInjected()) return;

            libmem::Process process = AsphaltTas::MemoryUtility::GetAsphaltProcessOrThrow();
            HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.pid);

            if (!process_handle) 
            {
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): OpenProcess failed");
            }

            g_game_directory_path = WideStringToString(GetProcessDirectory(process_handle));

            g_dll_path_w_string = ResolveFullDLLPath();
            g_dll_path_string  = WideStringToString(g_dll_path_w_string);
            SIZE_T size = (g_dll_path_w_string.size() + 1) * sizeof(wchar_t);
            LPVOID remote_buffer = VirtualAllocEx(process_handle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            if (! remote_buffer) 
            {
                CloseHandle(process_handle);
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): VirtualAllocEx failed");
            }

            if (! WriteProcessMemory(process_handle, remote_buffer, g_dll_path_w_string.c_str(), size, nullptr)) 
            {
                VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE);
                CloseHandle(process_handle);
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): WriteProcessMemory failed");
            }

            LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

            if (!loadLibraryAddr) 
            {
                VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE);
                CloseHandle(process_handle);
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): GetProcAddress failed");
            }

            HANDLE thread_handle = CreateRemoteThread(process_handle, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remote_buffer, 0, nullptr);

            if (!thread_handle) 
            {
                VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE);
                CloseHandle(process_handle);
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): CreateRemoteThread failed");
            }

            WaitForSingleObject(thread_handle, INFINITE);

            DWORD hLibModule = 0;
            GetExitCodeThread(thread_handle, &hLibModule);

            VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE);
            CloseHandle(thread_handle);
            CloseHandle(process_handle);

            if (hLibModule == 0) 
            {
                throw std::runtime_error("At AsphaltDLLState::InjectIntoGame(): LoadLibraryW failed inside the target process.");
            }
            
            g_is_injected.store(true, std::memory_order::release);
        }

        void EjectFromGame()
        {
            if (!IsInjected()) return;
            
            g_is_injected.store(false, std::memory_order::release);

            g_dll_general_command.m_write_meta_data.m_request_dll_shutdown = true;
            g_dll_general_command.m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
            g_dll_path_string = "";
            g_dll_path_w_string = L"";
            g_game_directory_path = "";
            ComSharedMem::GetSharedState()->m_dll_in_buffer_general.PushOverwrite(g_dll_general_command);
        }

        bool IsInjected() noexcept
        {
            return g_is_injected.load(std::memory_order::acquire);
        }

        std::optional<std::string> GetGameDirectoryPath() noexcept
        {
            if (g_game_directory_path == "") return std::nullopt;
            return g_game_directory_path;
        }

    }
}