#include "globalstate/AsphaltDllManager.h"

#include "AsphaltDllManager.h"
#include "BulletDebugDrawStream.h"
#include "Communication.h"
#include "core/model/Mesh.h"
#include "core/utility/Assert.h"
#include "core/utility/Timer.h"
#include "core/utility/Units.h"
#include "glm/ext/vector_float3.hpp"
#include "layer/TasInputLayer.h"
#include "memory/MemoryUtility.h"
#include "common/Utility.h"

#include <atomic>
#include <memory>
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

        inline ComDllIn::DllGeneralCommandsIn g_dll_general_command;
        inline std::mutex g_dll_general_command_mutex;

        inline std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData> g_debug_frame_data;
        inline std::mutex g_debug_frame_data_mutex;

        inline std::unordered_map<uint64_t, std::vector<BulletTypes::DebugDrawStream::CachedMeshVertex>> g_pending_debug_mesh_assembly;
        inline std::mutex g_pending_debug_mesh_assembly_mutex;

        inline std::vector<CoreEngine::BulletDebugDraw_RenderPipeline::CompletedStaticMesh> g_completed_debug_draw_static_meshes;
        inline std::mutex g_completed_debug_draw_static_meshes_mutex;

        std::wstring g_dll_path_w_string  = L"";
        std::string g_dll_path_string     = "";
        std::string g_game_directory_path = "";

        void DrainStaticMeshChunks(ComSharedMem::SharedState* shared) noexcept
        {
            BulletTypes::DebugDrawStream::CachedMeshDefinitionChunk chunk;

            std::lock_guard<std::mutex> assembly_lock(g_pending_debug_mesh_assembly_mutex);

            while (shared->m_dll_out_debug_draw_static_meshes.TryPop(chunk))
            {
                std::vector<BulletTypes::DebugDrawStream::CachedMeshVertex>& verts = g_pending_debug_mesh_assembly[chunk.m_mesh_id];
                verts.insert(verts.end(), chunk.m_vertices, chunk.m_vertices + chunk.m_vertex_count);

                if (chunk.m_is_last_chunk)
                {
                    std::vector<CoreEngine::Vertex> vertices;
                    vertices.reserve(verts.size());

                    const auto ToGlmVec3ConvXYZ = [](const BulletTypes::Vector3& vec)
                    {
                        return glm::vec3 {vec.x, vec.z, -vec.y};
                    };

                    for (const auto& v : verts)
                    {
                        CoreEngine::Vertex vert_out;
                        vert_out.m_position = ToGlmVec3ConvXYZ(v.m_position);
                        vert_out.m_normal = ToGlmVec3ConvXYZ(v.m_normal);
                        vert_out.m_tex_uv = {0, 0};
                        vertices.push_back(vert_out);
                    }

                    std::lock_guard<std::mutex> completed_lock(g_completed_debug_draw_static_meshes_mutex);
                    CoreEngine::BulletDebugDraw_RenderPipeline::CompletedStaticMesh smesh_completeted;
                    g_completed_debug_draw_static_meshes.push_back({chunk.m_mesh_id, glm::vec3{chunk.m_color.x, chunk.m_color.y, chunk.m_color.z}, std::move(vertices)});
                    g_pending_debug_mesh_assembly.erase(chunk.m_mesh_id);
                }
            }
        }

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

            bool has_ack_quick_restart = false;

            while (shared->m_dll_out_buffer.TryPop(out_state))
            {
                const bool is_in_race          = out_state.m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE;
                bool new_race_began            = false;
                bool race_ended                = false;
                bool new_in_loading_screen     = false;
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

                ComDllOut::DllStateOut dummy;
                if (out_state.m_meta_data.m_race_status_state == Communication::DllOut::RaceStatusState::IN_QUICK_RESTART_PAUSE && ! shared->m_dll_out_buffer.TryPeek(dummy))
                {
                    TasInputLayer::OnRaceEnded();
                    TasInputLayer::OnRaceStarted();
                    has_ack_quick_restart = true;
                }
                
                TasInputLayer::OnDLLUpdate();
            }

            {
                ScopeLockedAccess<std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData>> prev_debug_draw_data = GetDllOutDebugFrameDataResultRef();

                std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData> debug_data = std::move(*prev_debug_draw_data);
                if (!debug_data)
                {
                    debug_data = std::make_unique<BulletTypes::DebugDrawStream::DebugFrameData>();
                }

                bool has_updated = false;

                while (shared->m_dll_out_debug_draw_stream.TryPop(*debug_data))
                {
                    has_updated = true;
                }

                static CoreEngine::Timer s_last_update {};
                static bool updated_once = false;

                if (has_updated)
                {
                    updated_once = true;
                    *prev_debug_draw_data = std::move(debug_data);
                    s_last_update.Restart();
                }
                else if (s_last_update.AtLeastElapsed(CoreEngine::Units::Second(10)))
                {
                    *prev_debug_draw_data = nullptr;
                }
                else
                {
                    //bool is_in_race = Utility::EqualsAny(GetDllStateOutCopy()->m_meta_data.m_race_status_state, Communication::DllOut::RaceStatusState::IN_RACE, Communication::DllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC);
                    *prev_debug_draw_data = (updated_once /*&& is_in_race*/) ? std::move(debug_data) : nullptr;
                }
            }
            
            DrainStaticMeshChunks(shared);

            ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> general_cmd = GetDllGeneralCommandsInRef();
            general_cmd->m_write_meta_data.m_command_type = ComDllIn::CommandType::ExecuteCommand;
            general_cmd->m_write_meta_data.m_acknowledge_quick_restart_pause = has_ack_quick_restart;

            shared->m_dll_in_buffer_general.PushOverwrite(*general_cmd);

            general_cmd->m_write_meta_data.m_request_track_reset = false; //Insure request runs only once
        }

        [[nodiscard]] std::vector<CoreEngine::BulletDebugDraw_RenderPipeline::CompletedStaticMesh> TakeNewlyCompletedStaticMeshes() noexcept
        {
            std::lock_guard<std::mutex> lock(g_completed_debug_draw_static_meshes_mutex);
            return std::exchange(g_completed_debug_draw_static_meshes, {});
        }

        ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>> GetDllStateOutLockResultRef() noexcept
        {
            return ScopeLockedAccess<std::optional<ComDllOut::DllStateOut>>(g_dll_state_out_mutex, g_dll_state_out);
        }

        ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn> GetDllGeneralCommandsInRef() noexcept
        {
            return ScopeLockedAccess<ComDllIn::DllGeneralCommandsIn>(g_dll_general_command_mutex, g_dll_general_command);
        }

        ScopeLockedAccess<std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData>> GetDllOutDebugFrameDataResultRef() noexcept
        {
            return ScopeLockedAccess<std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData>>(g_debug_frame_data_mutex, g_debug_frame_data);
        }

        std::optional<ComDllOut::DllStateOut> GetDllStateOutCopy() noexcept
        {
            std::scoped_lock<std::mutex> lock(g_dll_state_out_mutex);
            return g_dll_state_out;
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

            const auto path = std::filesystem::current_path();
            const std::wstring path_str = path.wstring();
            if (path_str.size() < Communication::SharedMemory::SharedState::EXTERNAL_TOOL_PATH_SIZE)
            {
                std::wmemcpy(shared->m_directory_external_tool, path_str.c_str(), static_cast<uint32_t>(path_str.size() + 1));
                shared->m_directory_external_tool_size = path_str.size();
            }
            else
            {
                shared->m_directory_external_tool[0] = L'\0';
                shared->m_directory_external_tool_size = 0;
            }
            
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