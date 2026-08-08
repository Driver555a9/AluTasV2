
#include "DetourFunctions.h"
#include "AsphaltDLL.h"
#include "AsphaltDLLUtility.h"
#include "Tests.h"
#include "BulletTypes.h"
#include "Communication.h"
#include "BulletSerializer.h"

#include <atomic>
#include <basetsd.h>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <XInput.h>
#include <cstdlib>
#include <cstring>
#include <excpt.h>
#include <array>
#include <fstream>
#include <ios>
#include <minwinbase.h>
#include <ostream>
#include <ranges>
#include <synchapi.h>
#include <thread>
#include <utility>
#include <algorithm>
#include <vector>

#include "MinHook.h"

#define REROUTE_FUNCTION __fastcall

namespace ComSharedMem = Communication::SharedMemory;
namespace ComDllIn = Communication::DllIn;
namespace ComDllOut = Communication::DllOut;

namespace AsphaltDLL
{
    namespace DetourFunctions 
    {
        ////////////////////////////////////////////////////////////
        // Implementation for generic memory R/W utilities
        ////////////////////////////////////////////////////////////
        namespace _Implementation
        {
            /// Detour_func               : called in place of actual func by hook
            /// Out_real_function_address : writes address of the real function in there. Used to enable or disable hook
            /// Out_trampoline            : writes address for calling original function
            bool SetupHook(LPCWSTR module_name, LPCSTR function_name, LPVOID detour_func, LPVOID* out_real_function_address, LPVOID* out_trampoline, std::atomic<HookState>& state) noexcept
            {
                if (state.load(std::memory_order::acquire) != HookState::NotInPlace)
                    return true;

                MH_STATUS status = MH_OK;

                status = MH_CreateHookApiEx(module_name, function_name, detour_func, out_trampoline, out_real_function_address);

                if (status != MH_OK)
                {
                    DLL_ERROR_PRINT("Failed to create Hook: Status: " << MH_StatusToString(status) << " Target: " 
                    << function_name << "() in module: " << Utility::LPCWSTRToString(module_name));
                    return false;
                }

                DLL_INFO_LOG("Successfully installed Hook for: " << function_name << "() in module: " << Utility::LPCWSTRToString(module_name));
                state.store(HookState::InPlaceDisabled, std::memory_order::release);
                return true;
            }

            bool SetupHook(LPCWSTR module_name, ULONG_PTR offset, LPVOID detour_func, LPVOID* out_real_function_address, LPVOID* out_trampoline, std::atomic<HookState>& state) noexcept
            {
                if (state.load(std::memory_order::acquire) != HookState::NotInPlace) return true;

                HMODULE module_hmod = GetModuleHandleW(module_name);
                if (!module_hmod)
                {
                    DLL_ERROR_PRINT("Failed to get module handle for: " << Utility::LPCWSTRToString(module_name));
                    return false;
                }

                uintptr_t base_address = reinterpret_cast<uintptr_t>(module_hmod);
                *out_real_function_address = reinterpret_cast<LPVOID>(base_address + offset);

                MH_STATUS status = MH_CreateHook(*out_real_function_address, detour_func, reinterpret_cast<void**>(out_trampoline));
                
                if (status != MH_OK)
                {
                    DLL_ERROR_PRINT("MinHook Error: " << MH_StatusToString(status) << " For Module: " 
                    << Utility::LPCWSTRToString(module_name) << " at Address: " << *out_real_function_address);
                    return false;
                }

                DLL_INFO_LOG("Successfully installed Hook in Module: " << Utility::LPCWSTRToString(module_name) 
                << " at offset: 0x" << std::hex << std::uppercase << offset << std::dec);
                state.store(HookState::InPlaceDisabled, std::memory_order::release);
                
                return true;
            }

            bool RemoveHook(LPVOID real_function_address, std::atomic<HookState>& state) noexcept
            {
                if (state.load(std::memory_order::acquire) == HookState::NotInPlace) return true;

                const MH_STATUS status = MH_RemoveHook(real_function_address);
                const bool success = status == MH_OK;

                if (success)
                {
                    state.store(HookState::NotInPlace, std::memory_order::release);
                    DLL_INFO_LOG("Successfully removed hook at address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }
                else
                {
                    DLL_ERROR_PRINT("Failed to remove Hook: " << MH_StatusToString(status) << " Address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }

                return success;
            }

            bool EnableHook(LPVOID real_function_address, std::atomic<HookState>& state) noexcept
            {
                if (state.load(std::memory_order::acquire) == HookState::InPlaceEnabled) return true;
                if (state.load(std::memory_order::acquire) == HookState::NotInPlace) return false;

                const MH_STATUS status = MH_EnableHook(real_function_address);
                const bool success = status == MH_OK;

                if (success)
                {
                    state.store(HookState::InPlaceEnabled, std::memory_order::release);
                    DLL_INFO_LOG("Successfully enabled hook at address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }
                else 
                {
                    DLL_ERROR_PRINT("Failed to enable Hook: " << MH_StatusToString(status) << " Address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }

                return success;
            }

            bool DisableHook(LPVOID real_function_address, std::atomic<HookState>& state) noexcept
            {
                if (state.load(std::memory_order::acquire) != HookState::InPlaceEnabled) return true;

                const MH_STATUS status = MH_DisableHook(real_function_address);
                const bool success = status == MH_OK;

                if (success)
                {
                    state.store(HookState::InPlaceDisabled, std::memory_order::release);
                    DLL_INFO_LOG("Successfully disabled hook at Address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }
                else 
                {
                    DLL_ERROR_PRINT("Failed to disable Hook: " << MH_StatusToString(status) << " Address: 0x" << std::hex << std::uppercase << real_function_address << std::dec);
                }

                return success;
            }

            void PatchMemory(LPCWSTR module_name, uintptr_t offset, const uint8_t* bytes, size_t size) noexcept
            {
                HMODULE module_hmod = GetModuleHandleW(module_name);
                if (!module_hmod)
                {
                    DLL_ERROR_PRINT("Failed to get module handle for: " << Utility::LPCWSTRToString(module_name));
                    return;
                }

                void* targetAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(module_hmod) + offset);
                
                DWORD oldProtect;
                if (VirtualProtect(targetAddress, size, PAGE_EXECUTE_READWRITE, &oldProtect))
                {
                    std::memcpy(targetAddress, bytes, size);
                    VirtualProtect(targetAddress, size, oldProtect, &oldProtect);
                }
                DLL_INFO_LOG("Successfully patched memory in module: " << Utility::LPCWSTRToString(module_name) << " Offset: 0x" << std::hex 
                << std::uppercase << offset << std::dec << " Size: " << size);
            }

            [[nodiscard]] uintptr_t GetMainGameModule() noexcept
            {
                static uintptr_t module = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Asphalt9_Steam_x64_rtl.exe"));
                return module;
            }
        } 

        namespace StateManager
        {
            std::atomic<bool> g_is_in_active_tick = false;

            // No mutex lock, caller must lock
            void OnHandleGeneralInBuffer() noexcept
            {
                ComSharedMem::SharedState* shared = ComSharedMem::GetSharedState();
                auto& in_buff = shared->m_dll_in_buffer_general;
                ComDllIn::DllGeneralCommandsIn command;

                if (shared->m_non_negotiable_communication_version != Communication::CURRENT_NON_NEGOTIABLE_COMMUNICATION_VERSION)
                {
                    static bool once = false;
                    if (!once)
                    {
                        DLL_ERROR_PRINT("Non negotiable communication version missmatch: Expected: " << Communication::CURRENT_NON_NEGOTIABLE_COMMUNICATION_VERSION
                                     << " Actual: " << shared->m_non_negotiable_communication_version << " Commands ignored.");
                        once = true; 
                    }
                    return;
                }

                while (in_buff.TryPop(command))
                {
                    const ComDllIn::WriteMetaData& meta_cmd = command.m_write_meta_data;
                    const ComDllIn::WriteRacerState& racer_cmd = command.m_write_racer_state;
                    const ComDllIn::WriteCameraState& camera_cmd = command.m_write_camera_state;

                    if (meta_cmd.m_command_type == ComDllIn::CommandType::ExecuteCommand)
                    {
                        if (meta_cmd.m_request_dll_shutdown)
                        {
                            RequestShutdown();
                        }

                        if ( GameDLLState::g_current_state.m_meta_data.m_replay_mode_status != Communication::ReplayMode::Inactive
                          && GameDLLState::g_current_state.m_meta_data.m_race_status_state  == ComDllOut::RaceStatusState::IN_RACE
                          && meta_cmd.m_replay_mode == Communication::ReplayMode::Inactive)
                        {
                            // We switched from replay to inactive - final frame
                            OnNewFrameWithPhysics::QueueSkipSubsequentTicks(GameDLLState::g_current_state.m_meta_data.m_on_replay_end_skip_tick_count);
                        }
                        
                        auto& curr_meta = GameDLLState::g_current_state.m_meta_data;
                            
                        curr_meta.m_physics_interval                        = meta_cmd.m_physics_interval;
                        curr_meta.m_fixed_frame_interval_micros             = std::clamp<uint32_t>(meta_cmd.m_fixed_frame_interval_micros, 4167, 33'333);
                        curr_meta.m_game_target_fps_interval_micros         = std::clamp<uint32_t>(meta_cmd.m_game_target_fps_interval_micros, 200, 66'666);
                        curr_meta.m_replay_mode_status                      = meta_cmd.m_replay_mode;
                        curr_meta.m_apply_physics_interval_override         = meta_cmd.m_apply_physics_interval_override; 
                        curr_meta.m_gui_is_hidden                           = meta_cmd.m_hide_gui;
                        curr_meta.m_skip_animation_flags                    = meta_cmd.m_skip_animation_flags;
                        curr_meta.m_replay_speed_factor                     = meta_cmd.m_replay_speed_factor;
                        curr_meta.m_on_replay_end_skip_tick_count           = meta_cmd.m_on_replay_end_skip_tick_count;  
                        curr_meta.m_speed_up_pre_race_cinematic             = meta_cmd.m_speed_up_pre_race_cinematic;   
                        curr_meta.m_dump_track_request_id                   = meta_cmd.m_dump_track_request_id;
                        curr_meta.m_speed_up_gui_animations                 = meta_cmd.m_speed_up_gui_animations;

                        const auto& resolved_addresses = GameDLLState::g_current_state.m_resolved_addresses;

                        if (resolved_addresses.m_game_target_fps_interval_address != NO_VALID_RESOLVED_ADDRESS)
                        {
                            *reinterpret_cast<uint32_t*>(resolved_addresses.m_game_target_fps_interval_address) = curr_meta.m_game_target_fps_interval_micros;
                        }
                    }

                    if (racer_cmd.m_command_type == ComDllIn::CommandType::ExecuteCommand)
                    {
                        const uintptr_t racer_base_addr = GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address;

                        // We set the values in our current state such that RacerUpdate can force this state if continuous override is on
                        GameDLLState::g_current_state.m_racer_state.m_continuous_override_on_flags = racer_cmd.m_continuous_override_on_flags;

                        std::memcpy(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4.Data(), 
                                    racer_cmd.m_racer_transform_mat4x4.Data(), sizeof(racer_cmd.m_racer_transform_mat4x4));

                        std::memcpy(GameDLLState::g_current_state.m_racer_state.m_racer_velocity_vec3.Data(), 
                                    racer_cmd.m_racer_velocity_vec3.Data(), sizeof(racer_cmd.m_racer_velocity_vec3));

                        if (racer_base_addr != NO_VALID_RESOLVED_ADDRESS)
                        {
                            std::memcpy(reinterpret_cast<void*>(racer_base_addr + ComDllIn::WriteRacerState::OFFSET_TRANSFORM), racer_cmd.m_racer_transform_mat4x4.Data(),
                                        sizeof(decltype(racer_cmd.m_racer_transform_mat4x4)));
                            
                            std::memcpy(reinterpret_cast<void*>(racer_base_addr + ComDllIn::WriteRacerState::OFFSET_VELOCITY), racer_cmd.m_racer_velocity_vec3.Data(),
                                        sizeof(decltype(racer_cmd.m_racer_velocity_vec3)));
                        } 
                        else 
                        {
                            DLL_ERROR_PRINT("Could not snap Racer to given transform because Racer base address is null.");
                        }

                        if (racer_cmd.m_nitro_bar_value >= 0.0f)
                        {
                            UpdateNitroBar::WriteNitroBar(racer_cmd.m_nitro_bar_value);
                        }
                    }   

                    if (camera_cmd.m_command_type == ComDllIn::CommandType::ExecuteCommand)
                    {
                        const uintptr_t camera_base_addr = GameDLLState::g_current_state.m_resolved_addresses.m_camera_state_base_address;

                        // We set the values in our current state such that CameraUpdate can force this state if continuous override is on
                        GameDLLState::g_current_state.m_camera_state.m_continuous_override_on_flags = camera_cmd.m_continuous_override_on_flags;

                        if (camera_cmd.m_continuous_override_on_flags & ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_FOV_RAD)
                        {
                            CameraUpdate::PatchDisableGameFovWriteInstruction();
                        }
                        else 
                        {
                            CameraUpdate::PatchEnableGameFovWriteInstruction();
                        }

                        std::memcpy(GameDLLState::g_current_state.m_camera_state.m_offset_relative_to_car.Data(), 
                                    camera_cmd.m_offset_relative_to_car.Data(), sizeof(camera_cmd.m_offset_relative_to_car));

                        std::memcpy(GameDLLState::g_current_state.m_camera_state.m_camera_position_vec3.Data(), 
                                    camera_cmd.m_camera_position_vec3.Data(), sizeof(camera_cmd.m_camera_position_vec3));

                        std::memcpy(GameDLLState::g_current_state.m_camera_state.m_camera_rotation_quat.Data(), 
                                    camera_cmd.m_camera_rotation_quat.Data(), sizeof(camera_cmd.m_camera_rotation_quat));

                        GameDLLState::g_current_state.m_camera_state.m_fov_radians = camera_cmd.m_fov_radians;
                        GameDLLState::g_current_state.m_camera_state.m_look_backwards = camera_cmd.m_look_backwards;
                        
                        if (camera_base_addr != NO_VALID_RESOLVED_ADDRESS)
                        {
                            std::memcpy(reinterpret_cast<void*>(camera_base_addr + ComDllIn::WriteCameraState::OFFSET_POSITON_VEC3), camera_cmd.m_camera_position_vec3.Data(),
                                        sizeof(decltype(camera_cmd.m_camera_position_vec3)));
                            
                            std::memcpy(reinterpret_cast<void*>(camera_base_addr + ComDllIn::WriteCameraState::OFFSET_ROTATION_QUAT), camera_cmd.m_camera_rotation_quat.Data(),
                                        sizeof(decltype(camera_cmd.m_camera_rotation_quat)));

                            *reinterpret_cast<float*>(camera_base_addr + ComDllIn::WriteCameraState::OFFSET_FOV_RADIANS) = camera_cmd.m_fov_radians;
                        }
                        else 
                        {
                            DLL_ERROR_PRINT("Could not snap Camera to given transform because Camera base address is null.");
                        }
                    }
                }
            }

            // No mutex lock, caller must lock
            void OnTryResolveNitroRCXArgPointer() noexcept
            {
                const auto status = GameDLLState::g_current_state.m_meta_data.m_race_status_state;

                if (GameDLLState::g_current_state.m_resolved_addresses.m_nitro_func_spoofed_rcx_arg != NO_VALID_RESOLVED_ADDRESS
                || ! (status == ComDllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC || status == ComDllOut::RaceStatusState::IN_RACE))
                {
                    return;
                }
                

                /////////////////////////////////////////////////
                // Use the pointer chains to try and resolve nitro argument pointer
                // If the chains converge, we assume this is the correct value
                /////////////////////////////////////////////////
                static const std::vector<uintptr_t> pointerchain_1 = {0x6EE7C08, 0x170, 0x68, 0x30, 0x0, 0x30, 0x28,0x38, 0x30 };
                static const std::vector<uintptr_t> pointerchain_2 = {0x6EE7C48, 0x170, 0x68, 0x38, 0x0, 0x58, 0x8, 0x30, 0x28, 0x88  };
                static const std::vector<uintptr_t> pointerchain_3 = {0x6EE7C48, 0x2C8, 0x48, 0x68, 0x30, 0x0, 0x30, 0x28, 0x38, 0x30 };
                const uintptr_t rcx_chain_1 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_1);
                const uintptr_t rcx_chain_2 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_2);
                const uintptr_t rcx_chain_3 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_3);

                if (rcx_chain_1 == rcx_chain_2 && rcx_chain_2 == rcx_chain_3)
                {
                    GameDLLState::g_current_state.m_resolved_addresses.m_nitro_func_spoofed_rcx_arg = rcx_chain_1;
                }
            }

            void OnTryResolveRespawnButtonRCXArgPointer() noexcept
            {
                const auto status = GameDLLState::g_current_state.m_meta_data.m_race_status_state;
                
                if (GameDLLState::g_current_state.m_resolved_addresses.m_nitro_func_spoofed_rcx_arg != NO_VALID_RESOLVED_ADDRESS
                || ! (status == ComDllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC || status == ComDllOut::RaceStatusState::IN_RACE))
                {
                    return;
                }

                static const std::vector<uintptr_t> pointerchain_1 = {0x6EE7C48, 0x40, 0x10, 0x28, 0x58, 0x30, 0xC0 };
                static const std::vector<uintptr_t> pointerchain_2 = {0x6EE7C08, 0x378, 0x10, 0x10, 0x8, 0x30, 0x40 };
                static const std::vector<uintptr_t> pointerchain_3 = {0x6EE7C08, 0x18, 0x8, 0x0, 0x28, 0x20, 0x8, 0x30 };
                const uintptr_t rcx_chain_1 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_1) + 0x150;
                const uintptr_t rcx_chain_2 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_2) + 0x38;
                const uintptr_t rcx_chain_3 = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_3) + 0x1E0;

                if (rcx_chain_1 == rcx_chain_2 && rcx_chain_2 == rcx_chain_3)
                {
                    GameDLLState::g_current_state.m_resolved_addresses.m_respawn_func_spoofed_rcx_arg = rcx_chain_1;
                }
            }

            // No mutex lock, caller must lock
            void OnTryResolveFpsTargetIntervalPointer() noexcept
            {
                auto& fps_addr = GameDLLState::g_current_state.m_resolved_addresses.m_game_target_fps_interval_address;
                if (fps_addr != NO_VALID_RESOLVED_ADDRESS)
                {
                    return;
                }

                static const std::vector<uintptr_t> pointerchain_1 = {0x06EE14E0, 0xFC8};
                fps_addr = Utility::SafeResolvePointerChain(_Implementation::GetMainGameModule(), pointerchain_1) + 0xC9C;

                if (! Utility::SafeDereference(reinterpret_cast<uint32_t*>(fps_addr)))
                {
                    fps_addr = NO_VALID_RESOLVED_ADDRESS;
                }

                if (fps_addr != NO_VALID_RESOLVED_ADDRESS)
                {
                    GameDLLState::g_current_state.m_meta_data.m_game_target_fps_interval_micros = *reinterpret_cast<uint32_t*>(GameDLLState::g_current_state.m_resolved_addresses.m_game_target_fps_interval_address);
                }
            }

            // No mutex lock, caller must lock
            void OnNewTick() noexcept
            {
                if (g_is_in_active_tick.load(std::memory_order::acquire)) return;

                GameDLLState::g_replay_current_frame_inputs = std::nullopt;

                OnTryResolveNitroRCXArgPointer();
                OnTryResolveFpsTargetIntervalPointer();
                OnTryResolveRespawnButtonRCXArgPointer();
                OnHandleGeneralInBuffer();

                if (GameDLLState::g_current_state.m_meta_data.m_race_status_state != ComDllOut::RaceStatusState::IN_RACE)
                {
                    return;
                }

                const uint32_t current_tick = GameDLLState::g_current_state.m_replay_inputs.m_race_frame_tick;
                const auto replay_mode= GameDLLState::g_current_state.m_meta_data.m_replay_mode_status;
                ComSharedMem::SharedState* shared = ComSharedMem::GetSharedState();
                auto& replay_buffer = shared->m_dll_in_buffer_input_replay;

                if (shared->m_non_negotiable_communication_version != Communication::CURRENT_NON_NEGOTIABLE_COMMUNICATION_VERSION)
                {
                    static bool once = false;
                    if (!once)
                    {
                        DLL_ERROR_PRINT("Non negotiable communication version missmatch: Expected: " << Communication::CURRENT_NON_NEGOTIABLE_COMMUNICATION_VERSION
                                     << " Actual: " << shared->m_non_negotiable_communication_version << " Replay Inputs ignored.");
                        once = true; 
                    }
                    return;
                }

                if (replay_mode == Communication::ReplayMode::ActiveBlockThread)
                {
                    while (true)
                    {
                        OnHandleGeneralInBuffer();
                        if (GameDLLState::g_current_state.m_meta_data.m_replay_mode_status != Communication::ReplayMode::ActiveBlockThread) break;

                        ComDllIn::DllReplayInputIn pkt;
                        if (replay_buffer.TryPeek(pkt))
                        {
                            if (pkt.m_race_frame_tick == current_tick)
                            {
                                (void)replay_buffer.TryPop(pkt);
                                GameDLLState::g_replay_current_frame_inputs = pkt;
                                break;
                            }
                            else if (pkt.m_race_frame_tick < current_tick)
                            { 
                                (void)replay_buffer.TryPop(pkt); 
                                continue; 
                            }
                            else break;
                        }
                    }
                }
                else if (replay_mode == Communication::ReplayMode::ActiveNoBlock)
                {
                    ComDllIn::DllReplayInputIn pkt;
                    if (replay_buffer.TryPeek(pkt))
                    {
                        if (pkt.m_race_frame_tick == current_tick)
                        { 
                            (void)replay_buffer.TryPop(pkt); GameDLLState::g_replay_current_frame_inputs = pkt; 
                        }
                        else if (pkt.m_race_frame_tick < current_tick)
                        {
                            (void)replay_buffer.TryPop(pkt);
                        }
                    }
                }
            }

            // No mutex lock, caller must lock
            void OnEndTick() noexcept 
            {
                GameDLLState::g_current_state.IncreasePacketIDToHighest();
                ComSharedMem::GetSharedState()->m_dll_out_buffer.PushOverwrite(GameDLLState::g_current_state);

                // Reset tick specific input state that is not updated per tick on its own
                GameDLLState::g_current_state.m_replay_inputs.m_barrel_angular_velocities_vec3    = {0,0,0};
                GameDLLState::g_current_state.m_replay_inputs.m_value_rbx_2228                    = 0;
                GameDLLState::g_current_state.m_replay_inputs.m_value_rbx_222C                    = 0;
                GameDLLState::g_current_state.m_replay_inputs.m_respawn_button_press              = false;
                GameDLLState::g_current_state.m_replay_inputs.m_nitro_activation_count_this_frame = 0;

                if (GameDLLState::g_current_state.m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE)
                {
                    GameDLLState::g_current_state.m_replay_inputs.m_race_frame_tick++;
                }
                g_is_in_active_tick.store(false, std::memory_order::release);
            }

            // Does not lock, caller must lock
            bool OnExportTrack() noexcept
            {
                const std::vector<BVHBroadphaseTraversal::DumpedNode> leaves = BVHBroadphaseTraversal::DumpAllLeaves();
                if (leaves.empty())
                {
                    DLL_ERROR_PRINT("OnExportTrack(): Empty leaves after BVH traversal.");
                    return false;
                }
                DLL_INFO_LOG("Serializing " << leaves.size() << " leaves to objects.TRACK");

                const std::vector<std::pair<BulletTypes::CollisionObject*, bool>> objects = std::ranges::to<std::vector>(leaves 
                    | std::ranges::views::transform([](const BVHBroadphaseTraversal::DumpedNode& e) -> std::pair<BulletTypes::CollisionObject*, bool> 
                    { return {e.m_broadphase_proxy->m_client_body, e.m_is_from_static_tree}; }));
                BulletTypes::Serializer::SerializeObjectsToFile(objects, Communication::DLL_DUMPED_TRACK_FILE_NAME);

                return true;
            }
        }

        namespace NewLogicTickDispatcher
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (*NewLogicTickDispatcher_t)(uintptr_t rcx, uintptr_t rdx);
                NewLogicTickDispatcher_t RealNewLogicTickDispatcherCall = nullptr;

                void REROUTE_FUNCTION Detour_NewLogicTickDispatcher(uintptr_t rcx, uintptr_t rdx)
                {
                    std::uint32_t iterations = 0;
                    
                    enum class FrameExecutionMode 
                    {
                        StandardFallback,
                        IntroCinematicSkip,
                        AccumulatorNormalRace,
                        AccumulatorReplayFastForward,
                        IsPausedOnlyOneTick
                    };
                    
                    FrameExecutionMode current_mode = FrameExecutionMode::StandardFallback;

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        GameDLLState::g_current_state.m_resolved_addresses.m_is_paused_func_spoofed_rcx_arg = reinterpret_cast<uintptr_t*>(rcx)[26];
                        GameDLLState::g_current_state.m_meta_data.m_is_currently_paused = IsPaused::GetIsPaused();

                        if (GameDLLState::g_current_state.m_meta_data.m_dump_track_request_id > GameDLLState::g_current_state.m_meta_data.m_last_completed_dump_request_id
                        && (GameDLLState::g_current_state.m_meta_data.m_race_status_state == Communication::DllOut::RaceStatusState::IN_RACE 
                         || GameDLLState::g_current_state.m_meta_data.m_race_status_state == Communication::DllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC))
                        {
                            if (StateManager::OnExportTrack())
                            {
                                GameDLLState::g_current_state.m_meta_data.m_last_completed_dump_request_id = GameDLLState::g_current_state.m_meta_data.m_dump_track_request_id; 
                            }
                            else 
                            {
                                DLL_ERROR_PRINT("Failed to export track, despite request.");
                            }
                        }
                        
                        const auto status = GameDLLState::g_current_state.m_meta_data.m_race_status_state;
                        const bool speed_up_cin = GameDLLState::g_current_state.m_meta_data.m_speed_up_pre_race_cinematic;
                        const bool is_replay = (GameDLLState::g_current_state.m_meta_data.m_replay_mode_status != Communication::ReplayMode::Inactive);
                        const uint32_t replay_speed = GameDLLState::g_current_state.m_meta_data.m_replay_speed_factor;
                        const uint32_t speed_factor = is_replay ? replay_speed : 1;

                        if (IsPaused::GetIsPaused())
                        {
                            iterations   = 1;
                            current_mode = FrameExecutionMode::IsPausedOnlyOneTick;
                        }
                        else if (status == ComDllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC && speed_up_cin)
                        {
                            iterations = 10'000;
                            current_mode = FrameExecutionMode::IntroCinematicSkip;
                        }
                        else
                        {
                            current_mode = is_replay ? FrameExecutionMode::AccumulatorReplayFastForward : FrameExecutionMode::AccumulatorNormalRace;

                            static uint64_t last_time = Utility::GetMonotonicMicrosecondCount();
                            const  uint64_t time_now  = Utility::GetMonotonicMicrosecondCount();

                            const uint64_t elapsed = std::clamp<uint64_t>(time_now - last_time, 0, 250'000);
                            last_time = time_now; 

                            static int64_t accumulator = 0;
                            accumulator += (elapsed * speed_factor);
                            
                            const int64_t target_interval = GameDLLState::g_current_state.m_meta_data.m_game_target_fps_interval_micros;

                            if (target_interval > 0)
                            {
                                const int64_t ticks = accumulator / target_interval;

                                iterations += static_cast<int>(ticks);
                                accumulator -= ticks * target_interval;
                            }
                            else
                            {
                                iterations = 1;
                            }
                        }
                    }

                    for (std::uint32_t i = 0; i < iterations; ++i)
                    {
                        RealNewLogicTickDispatcherCall(rcx, rdx);

                        if (current_mode == FrameExecutionMode::AccumulatorNormalRace || current_mode == FrameExecutionMode::StandardFallback)
                        {
                            continue; 
                        }
                        {
                            LOCK_CURRENT_STATE_MUTEX();
                            if (current_mode == FrameExecutionMode::IntroCinematicSkip)
                            {
                                const auto status = GameDLLState::g_current_state.m_meta_data.m_race_status_state;
                                if (status != ComDllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC)
                                {
                                    break;
                                }
                            }
                            else if (current_mode == FrameExecutionMode::AccumulatorReplayFastForward)
                            {
                                if (!GameDLLState::g_replay_current_frame_inputs.has_value())
                                {
                                    break; 
                                }
                            }
                        }
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x4CF180; 

                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_NewLogicTickDispatcher), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealNewLogicTickDispatcherCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnNewFrameWithPhysics
        {
            namespace
            {
                inline std::atomic<HookState> g_hook_state = HookState::NotInPlace;

                inline LPVOID g_real_function_address = nullptr;

                inline uint32_t g_ticks_left_to_skip = 0;

                typedef uintptr_t* (REROUTE_FUNCTION* OnNewFrameWithPhysics_t)(void* p_this, uintptr_t* p_out_delta_micros, uintptr_t* p_in_delta_micros);
                inline OnNewFrameWithPhysics_t RealNewPhysicsFrameCall = nullptr;

                uintptr_t* Detour_OnNewFrameWithPhysics(void* p_this, uintptr_t* p_out_delta_micros, uintptr_t* p_in_delta_micros)
                {
                    // We skip this frame entirely if requested after replay end
                    if (g_ticks_left_to_skip > 0)
                    {
                        g_ticks_left_to_skip--;
                        *p_in_delta_micros  = 0;
                        *p_out_delta_micros = 0;
                        return p_out_delta_micros;
                    }

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        *p_in_delta_micros = GameDLLState::g_current_state.m_meta_data.m_fixed_frame_interval_micros;

                        if (GameDLLState::g_replay_current_frame_inputs.has_value())
                        {
                            if (! (GameDLLState::g_replay_current_frame_inputs->m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::NITRO_ACTIVATION))
                            {
                                EnableNitro::SpoofCallToEnableNitroFunction(GameDLLState::g_replay_current_frame_inputs->m_nitro_activation_count_this_frame);
                            }

                            if ( ! (GameDLLState::g_replay_current_frame_inputs->m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::RESPAWN_BUTTON)
                                && GameDLLState::g_replay_current_frame_inputs->m_respawn_button_press)
                            {
                                OnRespawnButtonPressed::SpoofCallToRespawnInputFunc();
                            }
                        }
                    }

                    (void)RealNewPhysicsFrameCall(p_this, p_out_delta_micros, p_in_delta_micros);

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        if (GameDLLState::g_current_state.m_resolved_addresses.m_steering_struct_gear_address != NO_VALID_RESOLVED_ADDRESS)
                        {
                            const auto gear_addr = GameDLLState::g_current_state.m_resolved_addresses.m_steering_struct_gear_address;
                            GameDLLState::g_current_state.m_racer_state.m_gear = *reinterpret_cast<uint32_t*>(gear_addr);
                            constexpr uintptr_t OFFSET_GEAR_TO_RPM = 0x118;
                            GameDLLState::g_current_state.m_racer_state.m_rpm = *reinterpret_cast<float*>(gear_addr + OFFSET_GEAR_TO_RPM);
                        }
                        StateManager::OnEndTick();
                    }

                    return p_out_delta_micros;
                }
            }

            void QueueSkipSubsequentTicks(uint32_t amount) noexcept
            {
                g_ticks_left_to_skip = amount;
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x4869C30;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnNewFrameWithPhysics), 
                                                  &g_real_function_address, reinterpret_cast<LPVOID*>(&RealNewPhysicsFrameCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }

        }

        namespace NewBulletPhysicsTick
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;

                LPVOID g_real_function_address = nullptr;

                typedef void* (REROUTE_FUNCTION* NewBulletPhysicsTickFunction_t)(uintptr_t p_this, float* p_delta_time, void* p_passthrough);
                NewBulletPhysicsTickFunction_t RealNewBulletPhysicsTickCall = nullptr;

                void* REROUTE_FUNCTION Detour_NewBulletPhysicsTick(uintptr_t p_this, float* p_delta_time, void* p_passthrough) noexcept
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_resolved_addresses.m_physics_world_instance_address = p_this;

                       //Tests::DebugDumpPhysicsWorldObjects("deubug_world_dump.txt");
                       //if (GameDLLState::g_current_state.m_replay_inputs.m_race_frame_tick == 50)
                       //{
                         //   Tests::ChangeMaterialsTest();
                       //}
                    }

                    void* ret = RealNewBulletPhysicsTickCall(p_this, p_delta_time, p_passthrough);

                    return ret;
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x555D850;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_NewBulletPhysicsTick), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealNewBulletPhysicsTickCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state);}
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace SteeringValue 
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* SteeringValue_t)(void* p_this, float* p_value_ptr);
                SteeringValue_t RealSteeringValueCall = nullptr;

                void REROUTE_FUNCTION Detour_SteeringValue(void* p_this, float* p_value_ptr) noexcept
                {
                    {  
                        LOCK_CURRENT_STATE_MUTEX();

                        GameDLLState::g_current_state.m_resolved_addresses.m_steer_func_spoofed_rcx_arg = reinterpret_cast<uintptr_t>(p_this);
                        if (GameDLLState::g_replay_current_frame_inputs.has_value() 
                        && ! (GameDLLState::g_replay_current_frame_inputs.value().m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::STEER))
                        {
                            *p_value_ptr = GameDLLState::g_replay_current_frame_inputs->m_steer_value;
                        }
       
                        GameDLLState::g_current_state.m_replay_inputs.m_steer_value = *p_value_ptr;
                    }
                    RealSteeringValueCall(p_this, p_value_ptr);
                }
            }

            // This must not lock mutex such that we avoid a deadlock
            bool SpoofCallToSteerValueFunction(float steer) noexcept
            {
                if (GameDLLState::g_current_state.m_resolved_addresses.m_steer_func_spoofed_rcx_arg == NO_VALID_RESOLVED_ADDRESS)
                {
                    DLL_ERROR_PRINT("Could not spoof call to steer value function because rcx arg is not resolved yet");
                    return false;
                }

                if (!RealSteeringValueCall)
                {
                    DLL_ERROR_PRINT("Could not spoof call to steer value function because hook is not in place");
                    return false;
                }

                RealSteeringValueCall(reinterpret_cast<void*>(GameDLLState::g_current_state.m_resolved_addresses.m_steer_func_spoofed_rcx_arg), &steer);
                return true;
            }

            bool SetupHook() noexcept 
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x494C9D0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_SteeringValue), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealSteeringValueCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace BrakeValue
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* BrakeStatus_t)(uintptr_t p_this, float* p_value_ptr);
                BrakeStatus_t RealBrakeValueCall = nullptr;

                void REROUTE_FUNCTION Detour_BrakeValue(uintptr_t p_this, float* p_value_ptr) noexcept
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_resolved_addresses.m_brake_func_spoofed_rcx_arg = p_this;
                        // See offsets in steering non static dword CT
                        GameDLLState::g_current_state.m_resolved_addresses.m_steering_struct_gear_address = p_this + 0x1530 - 0x1470;

                        ///////////// First called hooked func that only runs for logic begins tick
                        StateManager::OnNewTick();

                        if (GameDLLState::g_replay_current_frame_inputs.has_value()
                        && ! (GameDLLState::g_replay_current_frame_inputs.value().m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::BRAKE))
                        {
                            *p_value_ptr = GameDLLState::g_replay_current_frame_inputs->m_brake_value;
                        }

                        GameDLLState::g_current_state.m_replay_inputs.m_brake_value = *p_value_ptr;
                    }
                    RealBrakeValueCall(p_this, p_value_ptr);
                }
            }

            // This must not lock mutex such that we avoid a deadlock
            bool SpoofCallToBrakeValueFunction(float brake) noexcept
            {
                if (GameDLLState::g_current_state.m_resolved_addresses.m_brake_func_spoofed_rcx_arg == NO_VALID_RESOLVED_ADDRESS)
                {
                    DLL_ERROR_PRINT("Could not spoof call to brake value function because rcx arg is not resolved yet");
                    return false;
                }

                if (!RealBrakeValueCall)
                {
                    DLL_ERROR_PRINT("Could not spoof call to brake value function because hook is not in place");
                    return false;
                }

                RealBrakeValueCall(GameDLLState::g_current_state.m_resolved_addresses.m_brake_func_spoofed_rcx_arg, &brake);
                return true;
            }

            bool SetupHook() noexcept 
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x494C9C0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_BrakeValue), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealBrakeValueCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace AcceleratorValue
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* AcceleratorValue_t)(void* p_this, float* p_pass_through);
                AcceleratorValue_t RealAcceleratorValueCall = nullptr;

                void REROUTE_FUNCTION Detour_AcceleratorValue(void* p_this, float* p_pass_through) noexcept
                {
                    // Function modifies our value, so we must run it prior to modifying the value afterwards
                    RealAcceleratorValueCall(p_this, p_pass_through);   
                    float* accelerator_value = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(p_this) + 0x1D0);

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        if (GameDLLState::g_replay_current_frame_inputs.has_value()
                        && ! (GameDLLState::g_replay_current_frame_inputs.value().m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::ACCELERATOR))
                        {
                            *accelerator_value = GameDLLState::g_replay_current_frame_inputs->m_accelerator_value;
                        }

                        GameDLLState::g_current_state.m_replay_inputs.m_accelerator_value = *accelerator_value;
                    }  
                }
            }

            bool SetupHook() noexcept 
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49514C0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_AcceleratorValue), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealAcceleratorValueCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace EnableNitro 
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* NitroEnableFunction_t)(uintptr_t nitro_state_rcx);
                NitroEnableFunction_t RealNitroEnableCall = nullptr;

                void REROUTE_FUNCTION Detour_EnableNitro(uintptr_t nitro_state_rcx) noexcept
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_replay_current_frame_inputs.has_value())
                        {
                            // We prevent player from clicking nitro if we're in a replay frame
                            return;
                        }

                        GameDLLState::g_current_state.m_replay_inputs.m_nitro_activation_count_this_frame++;
                    }
                    RealNitroEnableCall(nitro_state_rcx);
                }

            }

            // This must not lock mutex such that we avoid a deadlock
            void SpoofCallToEnableNitroFunction(uint32_t count) noexcept
            {
                if (RealNitroEnableCall == nullptr)
                {
                    DLL_ERROR_PRINT("Could not spoof call to enable nitro: hook is not in place.");
                    return;
                }
                if (GameDLLState::g_current_state.m_resolved_addresses.m_nitro_func_spoofed_rcx_arg == NO_VALID_RESOLVED_ADDRESS)
                {
                    DLL_ERROR_PRINT("Could not spoof call to enable nitro: verify global rcx arg pointer is not null.");
                    return;
                }
                while (count-- > 0)
                {
                    __try 
                    {
                        RealNitroEnableCall(GameDLLState::g_current_state.m_resolved_addresses.m_nitro_func_spoofed_rcx_arg);
                        GameDLLState::g_current_state.m_replay_inputs.m_nitro_activation_count_this_frame++;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        return;
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x4939870;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_EnableNitro), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealNitroEnableCall), g_hook_state);
            }

            bool RemoveHook() noexcept { RealNitroEnableCall = nullptr; return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace DecreaseNitroBarFunc
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* DecreaseNitroBar_t)(uintptr_t rcx, uintptr_t rdx_passthrough);
                DecreaseNitroBar_t RealDecreaseNitroBarCall = nullptr;

                void Detour_DecreaseNitroBar(uintptr_t rcx, uintptr_t rdx_passthrough) noexcept 
                {
                    float nitro_before {};
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_resolved_addresses.m_nitro_bar_encrypted_address = rcx + 0x18C;
                        nitro_before = GameDLLState::g_current_state.m_racer_state.m_nitro_bar_value;
                    }

                    RealDecreaseNitroBarCall(rcx, rdx_passthrough);

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_current_state.m_racer_state.m_continuous_override_on_flags & ComDllIn::WriteRacerState::CONTINUOUS_OVERRIDE_NITRO_BAR)
                        {
                            UpdateNitroBar::WriteNitroBar(nitro_before);
                        }
                        GameDLLState::g_current_state.m_racer_state.m_nitro_bar_value = UpdateNitroBar::ReadNitroBar();
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49A1100;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, 
                    reinterpret_cast<LPVOID>(&Detour_DecreaseNitroBar), &g_real_function_address, reinterpret_cast<LPVOID*>(&RealDecreaseNitroBarCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

    namespace IncreaseNitroBarFunc
    {
        namespace 
        {
            std::atomic<HookState> g_hook_state = HookState::NotInPlace;
            LPVOID g_real_function_address = nullptr;

            typedef void (REROUTE_FUNCTION* IncreaseNitroBar_t)(uintptr_t rcx, uintptr_t rdx, int r8d, float amount);
            IncreaseNitroBar_t RealIncreaseNitroBarCall = nullptr;

            void Detour_IncreaseNitroBar(uintptr_t rcx, uintptr_t rdx, int r8d, float amount) noexcept 
            {
                float nitro_before {};
                
                {
                    LOCK_CURRENT_STATE_MUTEX();
                    GameDLLState::g_current_state.m_resolved_addresses.m_nitro_bar_encrypted_address = rcx + 0x18C;
                    nitro_before = GameDLLState::g_current_state.m_racer_state.m_nitro_bar_value;
                }

                RealIncreaseNitroBarCall(rcx, rdx, r8d, amount);

                {
                    LOCK_CURRENT_STATE_MUTEX();
                    if (GameDLLState::g_current_state.m_racer_state.m_continuous_override_on_flags & ComDllIn::WriteRacerState::CONTINUOUS_OVERRIDE_NITRO_BAR)
                    {
                        UpdateNitroBar::WriteNitroBar(nitro_before);
                    }
                    GameDLLState::g_current_state.m_racer_state.m_nitro_bar_value = UpdateNitroBar::ReadNitroBar();
                }
            }
        }

        bool SetupHook() noexcept
        {
            constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49A0A10; 
            
            return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_IncreaseNitroBar), 
                &g_real_function_address, reinterpret_cast<LPVOID*>(&RealIncreaseNitroBarCall), g_hook_state);
        }

        bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
        bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
        bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
        HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
    }

        namespace UpdateNitroBar 
        {
            // This must not lock mutex such that we avoid a deadlock
            bool WriteNitroBar(float value) noexcept
            {
                const uintptr_t target_addr = GameDLLState::g_current_state.m_resolved_addresses.m_nitro_bar_encrypted_address;
                
                if (target_addr == NO_VALID_RESOLVED_ADDRESS)
                {
                    return false;
                }

                FloatXorObfuscationSetter::EncryptToAddress(reinterpret_cast<void*>(target_addr), value);
                
                return true;
            }

            // This must not lock mutex such that we avoid a deadlock
            float ReadNitroBar() noexcept
            {
                const uintptr_t target_addr = GameDLLState::g_current_state.m_resolved_addresses.m_nitro_bar_encrypted_address;

                if (target_addr == NO_VALID_RESOLVED_ADDRESS)
                {
                    return 0.0f;
                }

                return FloatXorObfuscationGetter::DecryptFromAddress(reinterpret_cast<void*>(target_addr));
            }
        }

        namespace LocalRacerAccessPoint
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* UpdateTransform_t)(uintptr_t transform_rcx, uintptr_t rdx, uintptr_t r8, uintptr_t r9);
                UpdateTransform_t RealUpdateTransformCall = nullptr;

                void REROUTE_FUNCTION Detour_LocalRacerAccessPoint(uintptr_t transform_rcx, uintptr_t rdx, uintptr_t r8, uintptr_t r9) noexcept
                {
                    RealUpdateTransformCall(transform_rcx, rdx, r8, r9);

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        const uintptr_t base = *reinterpret_cast<uintptr_t*>(transform_rcx);

                        if (GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address == NO_VALID_RESOLVED_ADDRESS 
                           || GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address != base)
                        {
                            return; // We're not in the local racer dynamics object
                        }
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x636FAA4; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_LocalRacerAccessPoint), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealUpdateTransformCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace GetLocalRacerStruct
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* RealGetLocalRacerStruct_t)(uintptr_t rcx);
                RealGetLocalRacerStruct_t RealGetLocalRacerStructCall = nullptr;

                void Detour_GetLocalRacerStruct(uintptr_t rcx) noexcept 
                {
                    RealGetLocalRacerStructCall(rcx);

                    uintptr_t rbx_container = *reinterpret_cast<uintptr_t*>(rcx + 0x1B8);

                    uintptr_t local_player_ptr = *reinterpret_cast<uintptr_t*>(rbx_container + 0x08);

                    constexpr uintptr_t local_racer_offset = 0x90; //Assumed constant; Original CT deduces this dynamically

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address = *reinterpret_cast<uintptr_t*>(local_player_ptr + local_racer_offset);
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x532CD0; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_GetLocalRacerStruct), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealGetLocalRacerStructCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace CameraUpdate
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address      = nullptr;

                typedef void (REROUTE_FUNCTION* CameraUpdate_t)(uintptr_t rcx);
                CameraUpdate_t RealCameraUpdateCall = nullptr;

                // movss [rcx+00000128],xmm0
                constexpr uint8_t g_update_fov_instruction_original_bytes[8] = {0xF3, 0x0F, 0x11, 0x81, 0x28, 0x01, 0x00, 0x00};
                constexpr uint8_t g_nops_8[8] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

                std::atomic<bool> g_memory_is_patched = false;

                void SetCameraRelativeToLocalRacerWithOffset(BulletTypes::UnalignedVector3 offsets, bool look_backwards = false)
                {
                    const float right   = offsets.x;
                    const float forward = offsets.y;
                    const float up      = offsets.z;

                    if (GameDLLState::g_current_state.m_resolved_addresses.m_steering_struct_gear_address != NO_VALID_RESOLVED_ADDRESS)
                    {
                        // See offsets in steering CT
                        const uintptr_t steer_base = GameDLLState::g_current_state.m_resolved_addresses.m_steering_struct_gear_address + 0x1480;

                        const uintptr_t pos_x                = steer_base + 0x7F8;
                        const uintptr_t pos_y                = steer_base + 0x7FC;
                        const uintptr_t terrain_height       = steer_base + 0x800;
                        const uintptr_t height_above_terrain = steer_base + 0x818;

                        const auto racer_trans = std::bit_cast<BulletTypes::Transform>(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4);
                        BulletTypes::Quaternion rotation = Utility::RotationFromTransform(racer_trans);
                        
                        if (look_backwards)
                        {
                            const auto orig = rotation;
                            rotation.x = -orig.z;
                            rotation.y =  orig.w;
                            rotation.z =  orig.x;
                            rotation.w = -orig.y;
                        }

                        //TODO: Fix this - why must we inverse right?
                        BulletTypes::Vector3 world_offset = Utility::RotateVectorByQuaternion(rotation, { -1.0f * right, forward, up });

                        const float pos_x_val = *reinterpret_cast<float*>(pos_x);
                        const float pos_y_val = *reinterpret_cast<float*>(pos_y);
                        const float pos_z_val = *reinterpret_cast<float*>(terrain_height) + *reinterpret_cast<float*>(height_above_terrain);

                        GameDLLState::g_current_state.m_camera_state.m_camera_position_vec3 = 
                        {
                            pos_x_val + world_offset.x,
                            pos_y_val + world_offset.y,
                            pos_z_val + world_offset.z
                        };

                        GameDLLState::g_current_state.m_camera_state.m_camera_rotation_quat = 
                        {
                            rotation.x, 
                            rotation.y, 
                            rotation.z, 
                            rotation.w
                        };
                    }
                }

                void REROUTE_FUNCTION Detour_CameraUpdate(uintptr_t rcx) noexcept
                {
                    RealCameraUpdateCall(rcx);

                    const uintptr_t cam_actual_base   = *reinterpret_cast<uintptr_t*>(rcx + 0x28);
                    const uintptr_t cam_position_addr = cam_actual_base + 0x38;

                    {
                        LOCK_CURRENT_STATE_MUTEX();

                        GameDLLState::g_current_state.m_resolved_addresses.m_camera_state_base_address = cam_position_addr;

                        ComDllOut::RecordedCameraState& cam = GameDLLState::g_current_state.m_camera_state;
                        const auto& override_flags = cam.m_continuous_override_on_flags;

                        if (override_flags & ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_RELATIVE_TO_CAR)
                        {
                            SetCameraRelativeToLocalRacerWithOffset(cam.m_offset_relative_to_car, cam.m_look_backwards);
                            std::memcpy(reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_POSITON_VEC3), 
                                cam.m_camera_position_vec3.Data(), sizeof(cam.m_camera_position_vec3));
                            std::memcpy(reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_ROTATION_QUAT), 
                                cam.m_camera_rotation_quat.Data(), sizeof(cam.m_camera_rotation_quat));
                        }

                        if (override_flags & ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_POSITION)
                        {
                            std::memcpy(reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_POSITON_VEC3), 
                                cam.m_camera_position_vec3.Data(), sizeof(cam.m_camera_position_vec3));
                        }
                        else
                        {
                            std::memcpy(cam.m_camera_position_vec3.Data(), 
                                reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_POSITON_VEC3), sizeof(cam.m_camera_position_vec3));
                        }

                        if (override_flags & ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_ROTATION)
                        {
                            std::memcpy(reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_ROTATION_QUAT), 
                                cam.m_camera_rotation_quat.Data(), sizeof(cam.m_camera_rotation_quat));
                        }
                        else
                        {
                            std::memcpy(cam.m_camera_rotation_quat.Data(), 
                                reinterpret_cast<uint8_t*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_ROTATION_QUAT), sizeof(cam.m_camera_rotation_quat));
                        }

                        if (override_flags & ComDllIn::WriteCameraState::CONTINUOUS_OVERRIDE_FOV_RAD)
                        {
                            *reinterpret_cast<float*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_FOV_RADIANS) = cam.m_fov_radians;
                        }
                        else
                        {
                            cam.m_fov_radians = *reinterpret_cast<float*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_FOV_RADIANS);
                        } 

                        cam.m_aspect_ratio = *reinterpret_cast<float*>(cam_position_addr + ComDllIn::WriteCameraState::OFFSET_ASPECT_RATIO);
                    }

                }
            }

            void PatchDisableGameFovWriteInstruction() noexcept
            {
                if (! g_memory_is_patched.load(std::memory_order::acquire))
                {
                    _Implementation::PatchMemory(L"Asphalt9_Steam_x64_rtl.exe", 0x5DB61C, g_nops_8, sizeof(g_nops_8));
                    g_memory_is_patched.store(true, std::memory_order::release);
                }
            }

            void PatchEnableGameFovWriteInstruction() noexcept
            {
                if (g_memory_is_patched.load(std::memory_order::acquire))
                {
                    _Implementation::PatchMemory(L"Asphalt9_Steam_x64_rtl.exe", 0x5DB61C, g_update_fov_instruction_original_bytes, sizeof(g_update_fov_instruction_original_bytes));
                    g_memory_is_patched.store(false, std::memory_order::release);
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x5DB5C0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_CameraUpdate),
                                                  &g_real_function_address, reinterpret_cast<LPVOID*>(&RealCameraUpdateCall), g_hook_state);
            }

            bool RemoveHook()  noexcept { return _Implementation::RemoveHook (g_real_function_address, g_hook_state); }
            bool EnableHook()  noexcept { return _Implementation::EnableHook (g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            [[nodiscard]] HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace BarrelRollStabilization
        {
            namespace
            {
                inline std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                inline LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* BarrelRollStabilization_t)(uintptr_t rcx, uintptr_t rdx);
                inline BarrelRollStabilization_t RealBarrelRollStabilizationCall = nullptr;

                void REROUTE_FUNCTION Detour_BarrelRollStabilization(uintptr_t rcx, uintptr_t rdx) noexcept
                {
                    RealBarrelRollStabilizationCall(rcx, rdx);

                    float* rbx_plus_2228 = reinterpret_cast<float*>(rcx + 0x2228);
                    float* rbx_plus_222C = reinterpret_cast<float*>(rcx + 0x222C);

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_replay_current_frame_inputs.has_value() &&
                         ! (GameDLLState::g_replay_current_frame_inputs.value().m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::BARREL_RBX))
                        {
                            *rbx_plus_2228 = GameDLLState::g_replay_current_frame_inputs->m_value_rbx_2228;
                            *rbx_plus_222C = GameDLLState::g_replay_current_frame_inputs->m_value_rbx_222C;
                        }

                        GameDLLState::g_current_state.m_replay_inputs.m_value_rbx_2228 = *rbx_plus_2228;
                        GameDLLState::g_current_state.m_replay_inputs.m_value_rbx_222C = *rbx_plus_222C;
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49639D0; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_BarrelRollStabilization), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealBarrelRollStabilizationCall), g_hook_state
                );
            }

            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace BarrelYawStabilization
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* BarrelYawStabilization_t)(uintptr_t rcx, uintptr_t rdx);
                BarrelYawStabilization_t RealBarrelYawStabilizationCall = nullptr;

                void REROUTE_FUNCTION Detour_BarrelYawStabilization(uintptr_t rcx, uintptr_t rdx) noexcept
                {
                    const uintptr_t physics_body   = *reinterpret_cast<uintptr_t*>(rcx + 0x18);
                    const uintptr_t internal_state = *reinterpret_cast<uintptr_t*>(physics_body + 0x90);
                    float* angular_velocity = reinterpret_cast<float*>(internal_state + 0x170);

                    RealBarrelYawStabilizationCall(rcx, rdx);
                    
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_replay_current_frame_inputs.has_value() &&
                         ! (GameDLLState::g_replay_current_frame_inputs.value().m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::BARREL_ANGULAR))
                        {
                            std::memcpy(angular_velocity, GameDLLState::g_replay_current_frame_inputs->m_barrel_angular_velocities_vec3.Data(), 
                                    sizeof(decltype(GameDLLState::g_replay_current_frame_inputs->m_barrel_angular_velocities_vec3)));

                        }
                        std::memcpy(GameDLLState::g_current_state.m_replay_inputs.m_barrel_angular_velocities_vec3.Data(), angular_velocity, 
                                    sizeof(decltype(GameDLLState::g_current_state.m_replay_inputs.m_barrel_angular_velocities_vec3)));
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49662B0; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_BarrelYawStabilization), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealBarrelYawStabilizationCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace FinalRacerTransformWriter
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                using FinalRacerTransformWriter = char(*)(__m128** a1, int64_t* a2, int a3);
                FinalRacerTransformWriter RealFinalRacerTransformWriterCall = nullptr;

                char REROUTE_FUNCTION Detour_FinalRacerTransformWriter(__m128** a1, int64_t* a2, int a3)
                {
                    const char ret = RealFinalRacerTransformWriterCall(a1, a2, a3);

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address == NO_VALID_RESOLVED_ADDRESS) 
                        {
                            return ret;
                        }

                        const auto base = GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address;

                        if (GameDLLState::g_replay_current_frame_inputs.has_value() 
                        && ! (GameDLLState::g_replay_current_frame_inputs->m_skip_override_flags & ComDllIn::DllReplayInputIn::SkipOverride::TRANSFORM_FORCED))
                        {
                            const auto inputs   = GameDLLState::g_replay_current_frame_inputs.value();
                            BulletTypes::UnalignedTransform* curr_trans = reinterpret_cast<BulletTypes::UnalignedTransform*>(base + ComDllIn::WriteRacerState::OFFSET_TRANSFORM);
                            BulletTypes::UnalignedVector3* curr_velo    = reinterpret_cast<BulletTypes::UnalignedVector3*>(base + ComDllIn::WriteRacerState::OFFSET_VELOCITY);

                            if (*curr_trans != inputs.m_racer_transform_mat4x4 || *curr_velo != inputs.m_racer_velocity_vec3)
                            {
                                //DLL_INFO_LOG("Transform / Velocity missmatch at tick: " << inputs.m_race_frame_tick << " - Overriden with replay data.");
                                std::memcpy(curr_trans->Data(), inputs.m_racer_transform_mat4x4.Data(), sizeof(inputs.m_racer_transform_mat4x4));
                                std::memcpy(curr_velo->Data(), inputs.m_racer_velocity_vec3.Data(), sizeof(inputs.m_racer_velocity_vec3));
                            }
                        }

                        // If continuous override, write the value from current state, else record the new value by game
                        // Transform
                        if (GameDLLState::g_current_state.m_racer_state.m_continuous_override_on_flags & ComDllIn::WriteRacerState::CONTINUOUS_OVERRIDE_TRANSFORM)
                        {
                            std::memcpy(reinterpret_cast<void*>(base + ComDllIn::WriteRacerState::OFFSET_TRANSFORM), 
                                        GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4.Data(),
                                        sizeof(decltype(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4)));
                        }
                        else
                        {
                            std::memcpy(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4.Data(), 
                                    reinterpret_cast<void*>(base + ComDllOut::RecordedRacerState::OFFSET_TRANSFORM), 
                                    sizeof(decltype(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4)));
                        }

                        // Velocity
                        if (GameDLLState::g_current_state.m_racer_state.m_continuous_override_on_flags & ComDllIn::WriteRacerState::CONTINUOUS_OVERRIDE_VELOCITY)
                        {
                            std::memcpy(reinterpret_cast<void*>(base + ComDllIn::WriteRacerState::OFFSET_VELOCITY), 
                                        GameDLLState::g_current_state.m_racer_state.m_racer_velocity_vec3.Data(),
                                        sizeof(decltype(GameDLLState::g_current_state.m_racer_state.m_racer_velocity_vec3)));
                        }
                        else 
                        {
                            std::memcpy(GameDLLState::g_current_state.m_racer_state.m_racer_velocity_vec3.Data(), 
                                    reinterpret_cast<void*>(base + ComDllOut::RecordedRacerState::OFFSET_VELOCITY), 
                                    sizeof(decltype(GameDLLState::g_current_state.m_racer_state.m_racer_velocity_vec3)));
                        }
                    }

                    return ret;
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x636FDE0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_FinalRacerTransformWriter),
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealFinalRacerTransformWriterCall), g_hook_state);
            }
            bool RemoveHook() noexcept  { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept  { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnWreck
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                uint64_t g_wreck_call_index = 0;

                typedef void (REROUTE_FUNCTION* OnWreck_t)(uintptr_t rcx);
                OnWreck_t RealOnWreckCall = nullptr;

                void REROUTE_FUNCTION Detour_OnWreck(uintptr_t rcx) noexcept
                {
                    RealOnWreckCall(rcx);
                    g_wreck_call_index++;

                    /*
                    ////////////////  DEBUG
                    if (!rcx) return;

                    float probability_threshold_2 = *reinterpret_cast<float*>(rcx + 0x54);
                    float breakage_threshold_1    = *reinterpret_cast<float*>(rcx + 0x58);

                    uintptr_t parts_start = *reinterpret_cast<uintptr_t*>(rcx + 0x38);
                    uintptr_t parts_end   = *reinterpret_cast<uintptr_t*>(rcx + 0x40);


                    if (parts_start == 0 || parts_end == 0 || parts_start >= parts_end) 
                    {
                        return;
                    }

                    size_t part_counter = 0;
                    std::cout << "========= [Wreck session: " << (g_wreck_call_index-1) << "] =========\n";
                    std::cout << "Will Break Threshhold: " << breakage_threshold_1 << "\nWill Detach Treshhold: " << probability_threshold_2 << "\n";
                    uint32_t broken_mask {};
                    uint32_t detach_mask {};
                    for (uintptr_t current_part = parts_start; current_part < parts_end; current_part += 0x60, part_counter++)
                    {
                        uint8_t byte_0x44    = *reinterpret_cast<uint8_t*>(current_part + 0x44); // IS DETACHED FLAG
                        uint8_t byte_0x45    = *reinterpret_cast<uint8_t*>(current_part + 0x45); // IS BROKEN FLAG
                        uint8_t byte_0x46    = *reinterpret_cast<uint8_t*>(current_part + 0x46); 
                        uintptr_t rigid_body = *reinterpret_cast<uintptr_t*>(current_part + 0x08);

                        std::cout << "PART: " << part_counter << " IS_BROKEN : " << static_cast<int>(byte_0x45) << " IS_DETACHED : " << static_cast<int>(byte_0x44) << std::endl;
                        broken_mask |= (byte_0x45 << part_counter);
                        detach_mask |= (byte_0x44 << part_counter); 
                    }
                    std::cout << "Broken Mask: " << std::hex << std::uppercase << broken_mask << "\nDetach Mask: " << detach_mask << "\n==========================" << std::dec << std::endl;

                    //////////// 
                    */
                }
            }

            std::uint64_t GetMonotonicWreckSessionCount() noexcept
            {
                return g_wreck_call_index;
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x2AC2B0; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnWreck), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnWreckCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

    namespace OnRespawnButtonPressed
    {
        namespace
        {
            std::atomic<HookState> g_hook_state = HookState::NotInPlace;
            LPVOID g_real_function_address = nullptr;

            using OnRespawnButtonPressed_t = uintptr_t(__fastcall*)(uintptr_t rcx);
            OnRespawnButtonPressed_t RealOnRespawnButtonPressedCall = nullptr;  

            uintptr_t REROUTE_FUNCTION Detour_OnRespawnButtonPressed(uintptr_t rcx) noexcept
            {
                const void* ret_addr = _ReturnAddress();
                constexpr uintptr_t STATIC_OFFSET_CALLER = 0x5B5D0C;
                const uintptr_t expected_return_address  = _Implementation::GetMainGameModule() + STATIC_OFFSET_CALLER;

                uintptr_t real_ret = RealOnRespawnButtonPressedCall(rcx);

                if (reinterpret_cast<uintptr_t>(ret_addr) == expected_return_address)
                {
                    LOCK_CURRENT_STATE_MUTEX();
                    if (rcx == GameDLLState::g_current_state.m_resolved_addresses.m_respawn_func_spoofed_rcx_arg)
                    {
                        GameDLLState::g_current_state.m_replay_inputs.m_respawn_button_press = true;
                    } 
                }

                return real_ret;
            }
        }

        void SpoofCallToRespawnInputFunc() noexcept
        {
            if (RealOnRespawnButtonPressedCall == nullptr)
            {
                DLL_ERROR_PRINT("Could not spoof call to respawn button: hook not in place.");
                return;
            }
            if (GameDLLState::g_current_state.m_resolved_addresses.m_respawn_func_spoofed_rcx_arg == NO_VALID_RESOLVED_ADDRESS)
            {
                DLL_ERROR_PRINT("Could not spoof call to respawn button: verify global rcx arg pointer is not null.");
                return;
            }
            
            __try 
            {
                RealOnRespawnButtonPressedCall(GameDLLState::g_current_state.m_resolved_addresses.m_respawn_func_spoofed_rcx_arg);
                GameDLLState::g_current_state.m_replay_inputs.m_respawn_button_press = true;
            } 
            __except (EXCEPTION_EXECUTE_HANDLER)
            {

            }
        }

        bool SetupHook() noexcept
        {
            constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x250390; 
            return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnRespawnButtonPressed), 
                                              &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnRespawnButtonPressedCall), g_hook_state);
        }

        bool RemoveHook() noexcept { RealOnRespawnButtonPressedCall = nullptr; return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
        bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
        bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
        
        HookState GetHookState() noexcept 
        { 
            return g_hook_state.load(std::memory_order::acquire); 
        }
    }

        namespace GetPhysicsInterval
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef float* (REROUTE_FUNCTION* GetPhysicsInterval_t)(void* p_this, float* p_out);
                GetPhysicsInterval_t RealGetPhysicsIntervalCall = nullptr;

                float* REROUTE_FUNCTION Detour_GetPhysicsInterval(void* p_this, float* p_out) noexcept
                {
                    RealGetPhysicsIntervalCall(p_this, p_out);

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        
                        if (GameDLLState::g_current_state.m_meta_data.m_apply_physics_interval_override)
                        {
                            *p_out = GameDLLState::g_current_state.m_meta_data.m_physics_interval;
                        }
                        
                        GameDLLState::g_current_state.m_meta_data.m_physics_interval = *p_out;
                    }
                    return p_out;
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x494F240; 

                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_GetPhysicsInterval), 
                                                  &g_real_function_address, reinterpret_cast<LPVOID*>(&RealGetPhysicsIntervalCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnBeginRaceFunction
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* BeginRaceFunction_t)(void* p_this);
                BeginRaceFunction_t RealOnBeginRaceFunctionCall = nullptr;

                void REROUTE_FUNCTION Detour_OnBeginRaceFunction(void* p_this) noexcept
                {
                    RealOnBeginRaceFunctionCall(p_this);

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_meta_data.m_race_status_state = ComDllOut::RaceStatusState::IN_RACE;
                        GameDLLState::g_current_state.m_resolved_addresses.ResetAll();
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x855940;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnBeginRaceFunction), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnBeginRaceFunctionCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnClickPlayFunction
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* OnClickPlayFunction_t)(uintptr_t rcx);
                OnClickPlayFunction_t RealOnClickPlayFunctionCall = nullptr;

                void REROUTE_FUNCTION Detour_OnClickPlayFunction(uintptr_t rcx) noexcept
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_meta_data.m_race_status_state = ComDllOut::RaceStatusState::IN_LOADING_SCREEN;
                    }
                    RealOnClickPlayFunctionCall(rcx);
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x6080A0;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnClickPlayFunction), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnClickPlayFunctionCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnEndRaceFunction
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef uintptr_t (REROUTE_FUNCTION* OnEndFunction_t)(void* p_this);
                OnEndFunction_t RealOnEndRaceFunctionCall = nullptr;

                uintptr_t REROUTE_FUNCTION Detour_OnEndRaceFunction(void* p_this) noexcept
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_meta_data.m_race_status_state   = ComDllOut::RaceStatusState::IN_MENU;
                        GameDLLState::g_current_state.m_replay_inputs.m_race_frame_tick = 0;
                        GameDLLState::g_current_state.m_resolved_addresses.ResetAll();
                    }
                    return RealOnEndRaceFunctionCall(p_this);
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x8A3580; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnEndRaceFunction), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnEndRaceFunctionCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnUpdateRaceProgress
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (REROUTE_FUNCTION* OnUpdateRaceProgress_t)(uintptr_t a1, float *a2, int64_t a3);
                OnUpdateRaceProgress_t RealOnUpdateRaceProgressCall = nullptr;

                void REROUTE_FUNCTION Detour_OnUpdateRaceProgress(uintptr_t a1, float *a2, int64_t a3) noexcept
                {
                    RealOnUpdateRaceProgressCall(a1, a2, a3);
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_racer_state.m_race_progress_percentage = *reinterpret_cast<float*>(a1 + 0x1D8) * 100.0f;
                    }
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x1259900; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnUpdateRaceProgress), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnUpdateRaceProgressCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace OnUpdateCheckpoint
        {
            namespace 
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef uintptr_t (REROUTE_FUNCTION* OnUpdateCheckpoint_t)(uintptr_t a1);
                OnUpdateCheckpoint_t RealOnUpdateCheckpointCall = nullptr;

                uintptr_t REROUTE_FUNCTION Detour_OnUpdateCheckpoint(uintptr_t a1) noexcept
                {
                    uintptr_t real_ret = RealOnUpdateCheckpointCall(a1);
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_racer_state.m_checkpoint = *reinterpret_cast<uint32_t*>(a1 + 0x24C);
                    }
                    return real_ret;
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x4995730; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnUpdateCheckpoint), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnUpdateCheckpointCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace FloatXorObfuscationSetter
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void* (REROUTE_FUNCTION* FloatXORSetter_t)(void* p_destination, float* p_source_value);
                FloatXORSetter_t RealFloatXORSetterCall = nullptr;

                void* REROUTE_FUNCTION Detour_FloatXORSetter(void* p_destination, float* p_source_value) noexcept
                {
                    return RealFloatXORSetterCall(p_destination, p_source_value);
                }
            } 

            // This must not lock mutex such that we avoid a deadlock
            void EncryptToAddress(void* p_dest, float value) noexcept 
            {
                RealFloatXORSetterCall(p_dest, &value); 
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x292420; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_FloatXORSetter), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealFloatXORSetterCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace FloatXorObfuscationGetter
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef float (*FloatXORGetter_t)(void* p_destination);
                FloatXORGetter_t RealFloatXORGetterCall = nullptr;

                float REROUTE_FUNCTION Detour_FloarXORGetter(void* p_destination)
                {
                    return RealFloatXORGetterCall(p_destination);
                }
            } 

            // This must not lock mutex such that we avoid a deadlock
            float DecryptFromAddress(void* original_addr) noexcept 
            {
                return RealFloatXORGetterCall(original_addr);
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x2A3EF0; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_FloarXORGetter), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealFloatXORGetterCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace RenderGUIToggle
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef uint32_t (*RenderGUIToggle_t)();
                RenderGUIToggle_t RealRenderGUIToggleCall = nullptr;

                uint32_t REROUTE_FUNCTION Detour_RenderGUIToggle()
                {
                    LOCK_CURRENT_STATE_MUTEX();
                    return GameDLLState::g_current_state.m_meta_data.m_gui_is_hidden ? 0 : 1;
                }
            } 

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x4B16D20; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_RenderGUIToggle), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealRenderGUIToggleCall), g_hook_state
                );
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace AnimationProgressFunction
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (*AnimationProgressFunction_t)(uintptr_t rcx, int64_t* rdx);
                AnimationProgressFunction_t RealAnimationProgressFunctionCall = nullptr;

                constexpr std::array<std::pair<Communication::SkipAnimationFlags, int64_t>, 5> banned_durations {
                    std::pair<Communication::SkipAnimationFlags, int64_t>{Communication::SkipAnimationFlags::SKIP_RACE_INTRO,        8217000},
                    std::pair<Communication::SkipAnimationFlags, int64_t>{Communication::SkipAnimationFlags::SKIP_RACE_INTRO,        1945000},
                    std::pair<Communication::SkipAnimationFlags, int64_t>{Communication::SkipAnimationFlags::SKIP_RACE_INTRO,        5500000},
                    std::pair<Communication::SkipAnimationFlags, int64_t>{Communication::SkipAnimationFlags::SKIP_RACE_INTRO,        3366666},
                    std::pair<Communication::SkipAnimationFlags, int64_t>{Communication::SkipAnimationFlags::SKIP_RACE_COUNT_DOWN,   2000000}
                };

                void REROUTE_FUNCTION Detour_AnimationProgressFunction(uintptr_t rcx, int64_t* rdx)
                {
                    const uintptr_t metadata_ptr  = *reinterpret_cast<uintptr_t*>(rcx + 8);
                    const int32_t internal_offset = *reinterpret_cast<int32_t*>(metadata_ptr + 4);
                    
                    const uintptr_t adjusted_rcx = rcx + internal_offset + 8;

                    const uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(adjusted_rcx);

                    using GetDuration_t = void(__fastcall*)(uintptr_t, int64_t*);
                    GetDuration_t GetDuration = reinterpret_cast<GetDuration_t>(vtable[12]);

                    int64_t duration = 0;
                    GetDuration(adjusted_rcx, &duration);

                    bool is_in_race = false;
                    uint32_t flags  = 0;

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        is_in_race = GameDLLState::g_current_state.m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_RACE;
                        flags = std::to_underlying(GameDLLState::g_current_state.m_meta_data.m_skip_animation_flags);

                        // If we previously were in loading screen and now match an animation duration for in race or countdown, we assume we're in pre race cinematic
                        if (GameDLLState::g_current_state.m_meta_data.m_race_status_state == ComDllOut::RaceStatusState::IN_LOADING_SCREEN)
                        {
                            for (const auto& it : banned_durations)
                            {
                                if ( (std::to_underlying(it.first) & std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_INTRO) || 
                                      std::to_underlying(it.first) & std::to_underlying(Communication::SkipAnimationFlags::SKIP_RACE_COUNT_DOWN)) &&
                                      duration == it.second)
                                {
                                    GameDLLState::g_current_state.m_meta_data.m_race_status_state = ComDllOut::RaceStatusState::IN_PRE_RACE_CINEMATIC;
                                }
                            }
                        }
                    }

                    //////////////////// Change this when there is animation skips inisde of race
                    if (! is_in_race)
                    {
                        for (const auto& it : banned_durations)
                        {
                            if ((flags & std::to_underlying(it.first)) && duration == it.second)
                            {
                                *rdx = duration;
                                break; 
                            }
                        }
                    }

                    RealAnimationProgressFunctionCall(rcx, rdx);
                }
            } 

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x2F37B40; 
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_AnimationProgressFunction), 
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealAnimationProgressFunctionCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace SpeedUpUIAnimations
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                using SpeedUpUIAnimations_t = uintptr_t (*)(uintptr_t a1, uint64_t ui_dt, uint64_t a3);
                SpeedUpUIAnimations_t RealSpeedUpUIAnimationsCall = nullptr;

                uintptr_t REROUTE_FUNCTION Detour_SpeedUpUIAnimations(uintptr_t a1, uint64_t ui_dt, uint64_t a3)
                {
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (GameDLLState::g_current_state.m_meta_data.m_speed_up_gui_animations)
                        {
                            if (GameDLLState::g_current_state.m_meta_data.m_race_status_state == Communication::DllOut::RaceStatusState::IN_RACE)
                            {
                                if (IsPaused::GetIsPaused())
                                {
                                    ui_dt *= 8;
                                }
                            }
                            else 
                            {
                                ui_dt *= 8;
                            }
                        }
                    }
                    return RealSpeedUpUIAnimationsCall(a1, ui_dt, a3);
                }
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x255290;
                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_SpeedUpUIAnimations),
                        &g_real_function_address, reinterpret_cast<LPVOID*>(&RealSpeedUpUIAnimationsCall), g_hook_state);
            }

            bool RemoveHook() noexcept  { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept  { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace IsPaused
        {
            bool GetIsPaused() noexcept
            {
                if (GameDLLState::g_current_state.m_resolved_addresses.m_is_paused_func_spoofed_rcx_arg == NO_VALID_RESOLVED_ADDRESS)
                {
                    return false;
                }
                using Fn = uint64_t(*)(uintptr_t rcx);
                Fn func = reinterpret_cast<Fn>(_Implementation::GetMainGameModule() + 0x4BACA0);

                return static_cast<bool>(func(GameDLLState::g_current_state.m_resolved_addresses.m_is_paused_func_spoofed_rcx_arg));
            }
        }

        namespace XInput_GetState
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;

                LPVOID g_real_function_address = nullptr;

                typedef DWORD (REROUTE_FUNCTION* XInputGetState_t)(DWORD user_index, XINPUT_STATE* state);
                XInputGetState_t RealXInputGetStateCall = nullptr;

                DWORD REROUTE_FUNCTION Detour_XInputGetState(DWORD user_index, XINPUT_STATE* state) noexcept
                {
                    if (!RealXInputGetStateCall) return ERROR_ASSERTION_FAILURE;

                    DWORD result = RealXInputGetStateCall(user_index, state);

                    if (result == ERROR_SUCCESS && state)
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        GameDLLState::g_current_state.m_xinput_state = ComDllOut::XInputState 
                        {
                            .m_packet_id     = state->dwPacketNumber,
                            .m_buttons       = state->Gamepad.wButtons,
                            .m_left_trigger  = state->Gamepad.bLeftTrigger,
                            .m_right_trigger = state->Gamepad.bRightTrigger,
                            .m_thumb_lx      = state->Gamepad.sThumbLX,
                            .m_thumb_ly      = state->Gamepad.sThumbLY,
                            .m_thumb_rx      = state->Gamepad.sThumbRX,
                            .m_thumb_ry      = state->Gamepad.sThumbRY
                        };
                    }
                    return result;
                }
            }

            bool SetupHook() noexcept
            {
                return _Implementation::SetupHook(L"xinput1_4.dll", "XInputGetState", reinterpret_cast<LPVOID>(&Detour_XInputGetState), &g_real_function_address, 
                    reinterpret_cast<LPVOID*>(&RealXInputGetStateCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state);}
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire);}
        }

        namespace UcrtBaseRand
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;

                LPVOID g_real_function_address = nullptr;

                typedef int (REROUTE_FUNCTION* UcrtBaseRand_t)();
                UcrtBaseRand_t RealUcrtBaseRandCall = nullptr;

                int REROUTE_FUNCTION Detour_UcrtBaseRand() noexcept
                {
                    ////////// TLS reset makes rand deterministic per race
                    const int natural_rand = RealUcrtBaseRandCall();
                    return natural_rand; 
                    //////////

                    /*
                    constexpr uintptr_t BEGIN_WRECK_FUNC_OFFSET_ABI_47_1_0 = 0x2AC2B0;
                    constexpr uintptr_t END_WRECK_FUNC_OFFSET_ABI_47_1_0   = 0x2AC9E2; 

                    constexpr uintptr_t FIRST_CALL_RETURN_ADDR_OFFSET_ABI_47_1_0  = 0x2AC39A;
                    constexpr uintptr_t SECOND_CALL_RETURN_ADDR_OFFSET_ABI_47_1_0 = 0x2AC7B4;

                    constexpr int DETACH_BREAKABLE        = 0;
                    constexpr int KEEP_BREAKABLE_ATTACHED = 32767;

                    const uintptr_t caller_address = reinterpret_cast<uintptr_t>(_ReturnAddress());

                    if (caller_address != (_Implementation::GetMainGameModule() + SECOND_CALL_RETURN_ADDR_OFFSET_ABI_47_1_0))
                    {
                        return 0; // Not related to detachable logic, unrandomized for visual consistency
                    }

                    static uint64_t s_active_wreck_session_count  = std::numeric_limits<uint64_t>::max(); // unitialized
                    static uint32_t s_call_count_this_session     = 0;

                    const uint64_t wreck_session_now = OnWreck::GetMonotonicWreckSessionCount();
                    s_call_count_this_session++;
                    
                    if (s_active_wreck_session_count != wreck_session_now) // init new session
                    {
                        s_active_wreck_session_count  = wreck_session_now;
                        s_call_count_this_session     = 0;
                    }

                    LOCK_CURRENT_STATE_MUTEX();
                    const auto mask = 0x0;
                    return (mask & (1 << s_call_count_this_session)) ? KEEP_BREAKABLE_ATTACHED : DETACH_BREAKABLE; */
                }
            }

            bool SetupHook() noexcept
            {
                return _Implementation::SetupHook(L"ucrtbase.dll", "rand", reinterpret_cast<LPVOID>(&Detour_UcrtBaseRand), &g_real_function_address, 
                    reinterpret_cast<LPVOID*>(&RealUcrtBaseRandCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state);}
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire);}
        }

        namespace BVHBroadphaseTraversal
        {
            namespace
            {
                std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                LPVOID g_real_function_address = nullptr;

                typedef void (__fastcall *BVHTraverse_t)(uintptr_t ctx_bvh, uintptr_t root_node, float* ray_origin, uintptr_t unused_a4, float* inv_ray_dir,
                                                        int* axis_signs, float max_distance, float* offset_min, float* offset_max, uintptr_t* callback_ctx );

                BVHTraverse_t RealBVHTraverseCall = nullptr;

                void REROUTE_FUNCTION Detour_BVHTraverse(uintptr_t ctx_bvh, uintptr_t root_node, float* ray_origin, uintptr_t unused_a4, float* inv_ray_dir, 
                                                            int* axis_signs, float max_distance, float* offset_min, float* offset_max, uintptr_t* callback_ctx) noexcept
                {
                    const uintptr_t ret_address = std::bit_cast<uintptr_t>(_ReturnAddress());
                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        if (_Implementation::GetMainGameModule() + 0x635EBF0 == ret_address)
                        {
                            GameDLLState::g_current_state.m_resolved_addresses.m_bvh_root_node_static_objects = root_node;
                        }
                        else if (_Implementation::GetMainGameModule() + 0x635EBAE == ret_address) 
                        {
                            GameDLLState::g_current_state.m_resolved_addresses.m_bvh_root_node_dynamic_objects = root_node;
                        }
                    }
                    RealBVHTraverseCall(ctx_bvh, root_node, ray_origin, unused_a4, inv_ray_dir, axis_signs, max_distance, offset_min, offset_max, callback_ctx);
                }

                std::vector<DumpedNode> _DumpAllLeaves(void* root_node, bool from_normal_tree) noexcept
                {
                    if (!root_node) return {};
                    std::vector<void*> stack;
                    std::vector<DumpedNode> out;
                    stack.push_back(root_node);

                    while (!stack.empty())
                    {
                        auto* node = reinterpret_cast<uint8_t*>(stack.back());
                        stack.pop_back();

                        float* min = reinterpret_cast<float*>(node + 0x00);
                        float* max = reinterpret_cast<float*>(node + 0x10);
                        int64_t child0 = *reinterpret_cast<int64_t*>(node + 0x28);
                        int64_t child1 = *reinterpret_cast<int64_t*>(node + 0x30);

                        if (child1 != 0)
                        {
                            stack.push_back(reinterpret_cast<void*>(child0));
                            stack.push_back(reinterpret_cast<void*>(child1));
                        }
                        else
                        {
                            out.push_back({ reinterpret_cast<decltype(DumpedNode::m_broadphase_proxy)>(child0),
                                            {min[0],min[1],min[2]},
                                            {max[0],max[1],max[2]}, from_normal_tree});

                        }
                    }
                    return out;
                }
            }

            // Does not lock - caller to lock
            std::vector<DumpedNode> DumpAllLeaves() noexcept 
            {
                void* root = nullptr;
                constexpr bool FROM_STATIC_TREE = true;
                {
                    root = std::bit_cast<void*>(GameDLLState::g_current_state.m_resolved_addresses.m_bvh_root_node_static_objects);
                }
                if (root == nullptr) return {};
                std::vector<DumpedNode> v1 = _DumpAllLeaves(root, FROM_STATIC_TREE);
                {
                    root = std::bit_cast<void*>(GameDLLState::g_current_state.m_resolved_addresses.m_bvh_root_node_dynamic_objects);
                }
                if (root == nullptr) return v1;
                std::vector<DumpedNode> v2 = _DumpAllLeaves(root, !FROM_STATIC_TREE);
                v1.insert(v1.end(), v2.begin(), v2.end());
                return v1;
            }

            bool SetupHook() noexcept
            {
                constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x635DC18; 

                return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_BVHTraverse),
                    &g_real_function_address, reinterpret_cast<LPVOID*>(&RealBVHTraverseCall), g_hook_state);
            }

            bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
            bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
            bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
            HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
        }

        namespace Experimental 
        {
            namespace PacingState
            {
                inline std::atomic<bool> g_should_render = true;
                inline int64_t g_last_time_micros = 0;
                inline int64_t g_accumulator_micros = 0;

                inline void SetShouldRender(bool render) noexcept
                {
                    g_should_render.store(render, std::memory_order_relaxed);
                }
                inline bool GetShouldRender() noexcept
                {
                    return g_should_render.load(std::memory_order_relaxed);
                }
            }

            namespace QueryPerformanceCounterHook
            {
                namespace
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    typedef BOOL(WINAPI* QueryPerformanceCounter_t)(LARGE_INTEGER* value);
                    QueryPerformanceCounter_t RealQueryPerformanceCounterCall = nullptr;

                    LARGE_INTEGER g_virtual_qpc{ 0 };
                    LARGE_INTEGER g_qpc_frequency{ 0 };
                    bool g_initialized = false;
                    std::mutex g_time_mutex;

                    BOOL WINAPI Detour_QueryPerformanceCounter(LARGE_INTEGER* value) noexcept
                    {
                        if (!value) return FALSE;

                        std::lock_guard<std::mutex> lock(g_time_mutex);

                        if (!g_initialized)
                        {
                            return RealQueryPerformanceCounterCall(value);
                        }

                        *value = g_virtual_qpc;
                        return TRUE;
                    }
                }

                void Initialize() noexcept
                {
                    if (!g_initialized && RealQueryPerformanceCounterCall)
                    {
                        QueryPerformanceFrequency(&g_qpc_frequency);
                        RealQueryPerformanceCounterCall(&g_virtual_qpc);
                        g_initialized = true;
                    }
                }

                void AdvanceVirtualTime(int64_t micros) noexcept
                {
                    std::lock_guard<std::mutex> lock(g_time_mutex);
                    if (!g_initialized) return;
                    LONGLONG ticks_to_add = (micros * g_qpc_frequency.QuadPart) / 1000000LL;
                    g_virtual_qpc.QuadPart += ticks_to_add;
                }

                void SyncToRealTime() noexcept
                {
                    std::lock_guard<std::mutex> lock(g_time_mutex);
                    if (g_initialized && RealQueryPerformanceCounterCall)
                    {
                        RealQueryPerformanceCounterCall(&g_virtual_qpc);
                    }
                }

                int64_t GetRealTimeMicros() noexcept
                {
                    if (!g_initialized || !RealQueryPerformanceCounterCall) return 0;
                    
                    LARGE_INTEGER counter;
                    RealQueryPerformanceCounterCall(&counter);
                    return 1000000LL * (counter.QuadPart / g_qpc_frequency.QuadPart) +
                        1000000LL * (counter.QuadPart % g_qpc_frequency.QuadPart) / g_qpc_frequency.QuadPart;
                }

                bool SetupHook() noexcept
                {
                    return _Implementation::SetupHook(L"KernelBase.dll", "QueryPerformanceCounter", reinterpret_cast<LPVOID>(&Detour_QueryPerformanceCounter), &g_real_function_address,
                        reinterpret_cast<LPVOID*>(&RealQueryPerformanceCounterCall), g_hook_state);
                }
                bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }

            namespace InitiateNewFrame
            {
                namespace
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    using InitiateNewFrame_t = uintptr_t (*)(uintptr_t a1, uint64_t a2, uint64_t a3);
                    InitiateNewFrame_t RealInitiateNewFrameCall = nullptr;

                    uintptr_t REROUTE_FUNCTION Detour_InitiateNewFrame(uintptr_t a1, uint64_t a2, uint64_t a3)
                    {
                        uint8_t* const p_skip_rendering = reinterpret_cast<uint8_t*>(a1 + 0x1D0);
                        
                        bool should_render = PacingState::GetShouldRender();

                        *p_skip_rendering = !should_render;

                        return RealInitiateNewFrameCall(a1, a2, a3);
                    }
                }

                bool SetupHook() noexcept
                {
                    constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x238E50;
                    return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_InitiateNewFrame),
                          &g_real_function_address, reinterpret_cast<LPVOID*>(&RealInitiateNewFrameCall), g_hook_state);
                }

                bool RemoveHook() noexcept  { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept  { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }

            namespace NewFrameSubscriberList
            {
                namespace 
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    typedef uintptr_t (REROUTE_FUNCTION* NewFrameSubscriberList_t)(uintptr_t a1);
                    NewFrameSubscriberList_t RealNewFrameSubscriberListCall = nullptr;

                    uintptr_t REROUTE_FUNCTION Detour_NewFrameSubscriberList(uintptr_t a1) noexcept
                    {
                        const uintptr_t expected_ret = _Implementation::GetMainGameModule() + 0x5E43308;
                        const uintptr_t ret = std::bit_cast<uintptr_t>(_ReturnAddress());

                        if (ret != expected_ret)
                        {
                            return RealNewFrameSubscriberListCall(a1);
                        }

                        //QueryPerformanceCounterHook::Initialize();
                        //QueryPerformanceCounterHook::AdvanceVirtualTime(16333);

                        //PacingState::SetShouldRender(true);
                        return RealNewFrameSubscriberListCall(a1);
                    }
                }

                bool SetupHook() noexcept
                {
                    constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x250390; 
                    return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_NewFrameSubscriberList), 
                          &g_real_function_address, reinterpret_cast<LPVOID*>(&RealNewFrameSubscriberListCall), g_hook_state);
                }

                bool RemoveHook() noexcept  { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept  { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }

            namespace MainFpsLimiter
            {
                namespace 
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    typedef void (REROUTE_FUNCTION* MainFpsLimiter_t)(uintptr_t a1);
                    MainFpsLimiter_t RealMainFpsLimiterCall = nullptr;

                    void REROUTE_FUNCTION Detour_MainFpsLimiter(uintptr_t a1) noexcept
                    {
                        RealMainFpsLimiterCall(a1);
                    }
                }

                bool SetupHook() noexcept
                {
                    constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x5E44F90; 
                    return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_MainFpsLimiter), 
                          &g_real_function_address, reinterpret_cast<LPVOID*>(&RealMainFpsLimiterCall), g_hook_state);
                }

                bool RemoveHook() noexcept  { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept  { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }

            namespace OnRaycastVehicleUpdate
            {
                namespace
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    typedef uint64_t* (*OnRaycastVehicleUpdate_t)(uint64_t a1, float *a2, uint64_t a3);
                    OnRaycastVehicleUpdate_t RealOnRaycastVehicleUpdateCall = nullptr;

                    uint64_t* REROUTE_FUNCTION Detour_OnRaycastVehicleUpdate(uint64_t a1, float *a2, uint64_t a3) noexcept
                    {
                        const uint64_t addr_ptr_to_array = a1 + 0x228;
                        const uint64_t* wheel_array_4 = *reinterpret_cast<uint64_t**>(addr_ptr_to_array);

                        constexpr uint64_t offset_direction_vector = 0x14;

                        //*reinterpret_cast<float*>(static_cast<uintptr_t>(wheel_array_4[0]) + offset_direction_vector + 2*sizeof(float)) = -0.05f;
                        //*reinterpret_cast<float*>(static_cast<uintptr_t>(wheel_array_4[1]) + offset_direction_vector + 2*sizeof(float)) = -0.05f;
                        //*reinterpret_cast<float*>(static_cast<uintptr_t>(wheel_array_4[2]) + offset_direction_vector + 2*sizeof(float)) = -0.3f;
                        //*reinterpret_cast<float*>(static_cast<uintptr_t>(wheel_array_4[3]) + offset_direction_vector + 2*sizeof(float)) = -0.3f;

                        //std::cout << "Wheels: " << std::hex << std::uppercase << wheel_array_4[0] << ", " << wheel_array_4[1] << ", " 
                        //<< wheel_array_4[2] << ", " << wheel_array_4[3] << std::dec << std::endl;
                        
                        return RealOnRaycastVehicleUpdateCall(a1, a2, a3);
                    }
                } 

                bool SetupHook() noexcept
                {
                    constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x49543D0; 
                    return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_OnRaycastVehicleUpdate), 
                        &g_real_function_address, reinterpret_cast<LPVOID*>(&RealOnRaycastVehicleUpdateCall), g_hook_state
                    );
                }

                bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }

            namespace PhysicsWorldRaycast
            {
                namespace 
                {
                    std::atomic<HookState> g_hook_state = HookState::NotInPlace;
                    LPVOID g_real_function_address = nullptr;

                    using PhysicsWorldRaycast_t = uint64_t(__fastcall*)(void* world, BulletTypes::RaycastOutput* output, float* start, 
                                                                        float* end, int16_t layer_mask, int16_t query_flags, int64_t* entity_filter);

                    PhysicsWorldRaycast_t RealPhysicsWorldRaycastCall = nullptr;

                    uint64_t REROUTE_FUNCTION Detour_PhysicsWorldRaycast(void* world, BulletTypes::RaycastOutput* output, float* start, 
                                                                        float* end, int16_t layer_mask, int16_t query_flags, int64_t* entity_filter) noexcept
                    {
                        return RealPhysicsWorldRaycastCall(world, output, start, end, layer_mask, query_flags, entity_filter);
                    }
                }

                BulletTypes::RaycastOutput SpoofCallToCastRay(BulletTypes::Vector3 start, BulletTypes::Vector3 end, uint16_t layer_mask, uint16_t query_flags) noexcept
                {
                    BulletTypes::RaycastOutput result{};

                    void* world = nullptr;

                    {
                        LOCK_CURRENT_STATE_MUTEX();
                        world = std::bit_cast<void*>(GameDLLState::g_current_state.m_resolved_addresses.m_physics_world_instance_address);
                    }

                    if (!world || !RealPhysicsWorldRaycastCall) return result;

                    int64_t no_filter_dummy = 0;

                    static_assert(sizeof(start) == 16 && sizeof(end) == 16, "Must never not be 16-aligned");

                    RealPhysicsWorldRaycastCall(world, &result, start.Data(), end.Data(), static_cast<int16_t>(layer_mask), 
                                                   static_cast<int16_t>(query_flags), &no_filter_dummy);

                    return result;
                }

                bool SetupHook() noexcept
                {
                    constexpr uintptr_t STATIC_OFFSET_ABI_47_1_0 = 0x555DBB0; 
                    return _Implementation::SetupHook(L"Asphalt9_Steam_x64_rtl.exe", STATIC_OFFSET_ABI_47_1_0, reinterpret_cast<LPVOID>(&Detour_PhysicsWorldRaycast), 
                        &g_real_function_address, reinterpret_cast<LPVOID*>(&RealPhysicsWorldRaycastCall), g_hook_state
                    );
                }

                bool RemoveHook() noexcept { return _Implementation::RemoveHook(g_real_function_address, g_hook_state); }
                bool EnableHook() noexcept { return _Implementation::EnableHook(g_real_function_address, g_hook_state); }
                bool DisableHook() noexcept { return _Implementation::DisableHook(g_real_function_address, g_hook_state); }
                HookState GetHookState() noexcept { return g_hook_state.load(std::memory_order::acquire); }
            }
        }
    }
}