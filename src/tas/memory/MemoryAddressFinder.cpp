#include "tas/memory/MemoryAddressFinder.h"

#include "libmem/libmem.hpp"

#include "tas/globalstate/GameState.h"
#include "tas/memory/MemoryUtility.h"

#include "core/utility/Timer.h"

#include <windows.h>

#include <array>
#include <cstring>
#include <thread>
#include <cmath>
#include <mutex>

namespace AsphaltTas
{
    constexpr static uintptr_t INVALID_ADDRESS = 0x0;

    namespace RacerCache
    {
        static std::mutex MUTEX;
        ////////////////////////////////////////
        // Original code
        ////////////////////////////////////////
        static libmem::Address ORIGINAL_CODE_ADDRESS = INVALID_ADDRESS;

        constexpr static size_t                        ORIGINAL_CODE_SIZE = 13;
        static std::array<uint8_t, ORIGINAL_CODE_SIZE> ORIGINAL_CODE{};
        static bool                                    ORIGINAL_CODE_CACHE_HAS_VALUE = false;

        ////////////////////////////////////////
        // Original code for offset
        ////////////////////////////////////////
        static libmem::Address ORIGINAL_CODE_FOR_OFFSET_ADDRESS = INVALID_ADDRESS;
        
        ////////////////////////////////////////
        // Cave
        ////////////////////////////////////////
        constexpr static size_t ALLOCATED_CAVE_SIZE    = 0x1000;
        static libmem::Address  ALLOCATED_CAVE_ADDRESS = INVALID_ADDRESS;

        ////////////////////////////////////////
        // Found pointer storage
        ////////////////////////////////////////
        constexpr static size_t ALLOCATED_POINTER_SIZE    = sizeof(uintptr_t);
        static libmem::Address  ALLOCATED_POINTER_ADDRESS = INVALID_ADDRESS;
    }

    uintptr_t MemoryAddressFinder::FindRacerStateBaseAddress(CoreEngine::Units::MilliSecond max_wait_time)
    {
        std::scoped_lock lock(RacerCache::MUTEX);
        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        ////////////////////////////////////////
        // Find hook location & save original code
        ////////////////////////////////////////
        if (RacerCache::ORIGINAL_CODE_ADDRESS == INVALID_ADDRESS)
        {
            RacerCache::ORIGINAL_CODE_ADDRESS = MemoryUtility::AOBScanOrThrow(&process, "48 89 43 08 F3 41 0F 10 8E 30 01 00 00", module.base, module.size);
        }

        if (! RacerCache::ORIGINAL_CODE_CACHE_HAS_VALUE)
        {
            MemoryUtility::ReadMemoryOrThrow(&process, RacerCache::ORIGINAL_CODE_ADDRESS, RacerCache::ORIGINAL_CODE.data(), RacerCache::ORIGINAL_CODE.size());
            RacerCache::ORIGINAL_CODE_CACHE_HAS_VALUE = true;
        }

        ////////////////////////////////////////
        // Allocate cave and capture storage
        ////////////////////////////////////////
        if (RacerCache::ALLOCATED_CAVE_ADDRESS == INVALID_ADDRESS)
        {
            RacerCache::ALLOCATED_CAVE_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, RacerCache::ALLOCATED_CAVE_SIZE, libmem::Prot::XRW);
        }

        if (RacerCache::ALLOCATED_POINTER_ADDRESS == INVALID_ADDRESS)
        {
            RacerCache::ALLOCATED_POINTER_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, RacerCache::ALLOCATED_POINTER_SIZE, libmem::Prot::RW);
        }

        ////////////////////////////////////////
        // Clear previous pointer
        ////////////////////////////////////////
        uintptr_t zero = 0;
        MemoryUtility::WriteMemoryOrThrow(&process, RacerCache::ALLOCATED_POINTER_ADDRESS, &zero, sizeof(zero));

        bool hook_installed = false;

        try 
        {
            ////////////////////////////////////////
            // Create trampoline
            ////////////////////////////////////////
            std::vector<uint8_t> trampoline_code;
            trampoline_code.reserve(RacerCache::ORIGINAL_CODE_SIZE + 20);

            trampoline_code.insert(trampoline_code.end(), {0x48, 0x89, 0x05, 0x00, 0x00, 0x00, 0x00});
            trampoline_code.insert(trampoline_code.end(), RacerCache::ORIGINAL_CODE.begin(), RacerCache::ORIGINAL_CODE.end());
            trampoline_code.insert(trampoline_code.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});

            const uint64_t ret_addr = static_cast<uint64_t>(RacerCache::ORIGINAL_CODE_ADDRESS + RacerCache::ORIGINAL_CODE_SIZE);
            trampoline_code.insert(trampoline_code.end(), reinterpret_cast<const uint8_t*>(&ret_addr), reinterpret_cast<const uint8_t*>(&ret_addr) + 8);

            // Patch disp32
            const uint64_t rip = static_cast<uint64_t>(RacerCache::ALLOCATED_CAVE_ADDRESS) + 7;
            const int64_t disp64 = static_cast<int64_t>(static_cast<uint64_t>(RacerCache::ALLOCATED_POINTER_ADDRESS) - rip);
            if (disp64 < INT32_MIN || disp64 > INT32_MAX) 
            {
                throw MemoryUtility::MemoryManipFailedException("RIP-relative displacement too large.");
            }
            const int32_t disp32 = static_cast<int32_t>(disp64);
            std::memcpy(&trampoline_code[3], &disp32, 4);

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, RacerCache::ALLOCATED_CAVE_ADDRESS, trampoline_code.data(), trampoline_code.size())) 
            {
                throw MemoryUtility::MemoryManipFailedException("Failed to write trampoline.");
            }

            ////////////////////////////////////////
            // Install Hook
            ////////////////////////////////////////
            std::vector<uint8_t> hook(13, 0x90);
            hook[0] = 0x49;
            hook[1] = 0xBB;
            std::memcpy(&hook[2], &RacerCache::ALLOCATED_CAVE_ADDRESS, 8);
            // jmp r11
            hook[10] = 0x41;
            hook[11] = 0xFF;
            hook[12] = 0xE3;

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, RacerCache::ORIGINAL_CODE_ADDRESS, hook.data(), RacerCache::ORIGINAL_CODE_SIZE)) 
            {
                throw MemoryUtility::MemoryManipFailedException("Failed to install hook.");
            }
            
            hook_installed = true;

            ////////////////////////////////////////
            // Wait for capture
            ////////////////////////////////////////
            uintptr_t captured_value = 0;
            CoreEngine::Units::MilliSecond deadline = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;

            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, RacerCache::ALLOCATED_POINTER_ADDRESS, reinterpret_cast<uint8_t*>(&captured_value), sizeof(captured_value))) 
                {
                    if (captured_value != 0) break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            ////////////////////////////////////////
            // Restore original code and free cave
            ////////////////////////////////////////
            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, RacerCache::ORIGINAL_CODE_ADDRESS, RacerCache::ORIGINAL_CODE.data(), RacerCache::ORIGINAL_CODE_SIZE)) 
            { 
                throw MemoryUtility::MemoryManipFailedException("Failed to restore original code.");
            }
            
            hook_installed = false;
 
            ////////////////////////////////////////
            // Check if a valid value was captured
            ////////////////////////////////////////
            if (captured_value == 0) 
            {
                throw MemoryUtility::MemoryManipFailedException("Failed to capture racer pointer after waiting.");
            }

            ////////////////////////////////////////
            // Find offset to pointer to base
            ////////////////////////////////////////
            if (RacerCache::ORIGINAL_CODE_FOR_OFFSET_ADDRESS == INVALID_ADDRESS)
            {
                RacerCache::ORIGINAL_CODE_FOR_OFFSET_ADDRESS 
                    = MemoryUtility::AOBScanOrThrow(&process, "48 8B ? ? ? ? ? 83 B8 04 01 00 00 ? 0F 95", module.base, module.size);
            }

            uint32_t offset = 0;
            if (! MemoryUtility::TryReadMemoryOrNothing(&process, RacerCache::ORIGINAL_CODE_FOR_OFFSET_ADDRESS + 3, reinterpret_cast<uint8_t*>(&offset), sizeof(offset))) 
            {
                throw MemoryUtility::MemoryManipFailedException("Failed to read racer base offset value.");
            }

            const uintptr_t struct_ptr       = captured_value;
            const uintptr_t base_ptr_address = struct_ptr + offset;
            uintptr_t racer_base = 0;

            CoreEngine::Units::MilliSecond deadline_2 = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;

            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline_2)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, base_ptr_address, reinterpret_cast<uint8_t*>(&racer_base), sizeof(racer_base))) 
                {
                    if (racer_base != 0) break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            return racer_base;
        }
        catch (...) 
        {
            ////////////////////////////////////////
            // Clean up if exception
            ////////////////////////////////////////
            if (hook_installed) 
            {
                MemoryUtility::TryWriteMemoryOrNothing(&process, RacerCache::ORIGINAL_CODE_ADDRESS, RacerCache::ORIGINAL_CODE.data(), RacerCache::ORIGINAL_CODE_SIZE);
            }
            throw;
        }
    }


    namespace CameraCache
    {
        static std::mutex MUTEX;
        ////////////////////////////////////////
        // Original code
        ////////////////////////////////////////
        static libmem::Address ORIGINAL_CODE_ADDRESS = INVALID_ADDRESS;

        constexpr static size_t                         ORIGINAL_CODE_SIZE = 14;
        static std::array<uint8_t, ORIGINAL_CODE_SIZE>  ORIGINAL_CODE;
        static bool                                     ORIGINAL_CODE_CACHE_HAS_VALUE = false;

        ////////////////////////////////////////
        // Cave
        ////////////////////////////////////////
        constexpr size_t       ALLOCATED_CAVE_SIZE    = 0x1000;
        static libmem::Address ALLOCATED_CAVE_ADDRESS = INVALID_ADDRESS;

        ////////////////////////////////////////
        // Found pointer storage
        ////////////////////////////////////////
        constexpr static size_t ALLOCATED_POINTER_SIZE    = sizeof(uintptr_t);
        static libmem::Address  ALLOCATED_POINTER_ADDRESS = INVALID_ADDRESS;
    }
    
    uintptr_t MemoryAddressFinder::FindCameraStateAddresses(CoreEngine::Units::MilliSecond max_wait_time)
    {
        std::scoped_lock lock(CameraCache::MUTEX);

        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        ////////////////////////////////////////
        // Find hook location & save original code
        ////////////////////////////////////////
        if (CameraCache::ORIGINAL_CODE_ADDRESS == INVALID_ADDRESS)
        {
            //?? = 56 as of 30.3.2026 - however, this is vulnerable to change, therefore best to leave ??
            CameraCache::ORIGINAL_CODE_ADDRESS = MemoryUtility::AOBScanOrThrow(&process, "F3 0F 10 08 F3 0F 10 50 04 F3 0F 5C ?? 78", module.base, module.size);
        }

        if (! CameraCache::ORIGINAL_CODE_CACHE_HAS_VALUE )
        {
            // Steal 14 bytes: movss xmm1,[rax] (4) + movss xmm2,[rax+04] (5) + subss xmm2,[rsi+78] (5)
            MemoryUtility::ReadMemoryOrThrow(&process, CameraCache::ORIGINAL_CODE_ADDRESS, CameraCache::ORIGINAL_CODE.data(), CameraCache::ORIGINAL_CODE_SIZE);
            CameraCache::ORIGINAL_CODE_CACHE_HAS_VALUE = true;
        }

        ////////////////////////////////////////
        // Allocate cave and pointer
        ////////////////////////////////////////
        if (CameraCache::ALLOCATED_CAVE_ADDRESS == INVALID_ADDRESS)
        {
            CameraCache::ALLOCATED_CAVE_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, CameraCache::ALLOCATED_CAVE_SIZE, libmem::Prot::XRW);
        }

        if (CameraCache::ALLOCATED_POINTER_ADDRESS == INVALID_ADDRESS)
        {
            CameraCache::ALLOCATED_POINTER_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, CameraCache::ALLOCATED_POINTER_SIZE, libmem::Prot::RW);
        }
        
        ////////////////////////////////////////
        // Clear previous
        ////////////////////////////////////////
        uintptr_t zero = 0;
        MemoryUtility::WriteMemoryOrThrow(&process, CameraCache::ALLOCATED_POINTER_ADDRESS, &zero, sizeof(zero));

        bool hook_installed {false};

        try 
        {
            ////////////////////////////////////////
            // Build trampoline
            ////////////////////////////////////////
            std::vector<uint8_t> trampoline_code;
            trampoline_code.reserve(CameraCache::ORIGINAL_CODE_SIZE + 20);

            trampoline_code.insert(trampoline_code.end(), {0x48, 0x89, 0x05, 0x00, 0x00, 0x00, 0x00});  // mov [rip+disp32], rax

            trampoline_code.insert(trampoline_code.end(), CameraCache::ORIGINAL_CODE.begin(), CameraCache::ORIGINAL_CODE.end());

            trampoline_code.insert(trampoline_code.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
            uint64_t ret_addr = static_cast<uint64_t>(CameraCache::ORIGINAL_CODE_ADDRESS + CameraCache::ORIGINAL_CODE_SIZE);
            trampoline_code.insert(trampoline_code.end(), reinterpret_cast<const uint8_t*>(&ret_addr), reinterpret_cast<const uint8_t*>(&ret_addr) + 8);

            // Patch RIP-relative disp for mov [rip+disp], rax
            uint64_t rip_for_disp = static_cast<uint64_t>(CameraCache::ALLOCATED_CAVE_ADDRESS) + 7;
            int64_t disp64 = static_cast<int64_t>(static_cast<uint64_t>(CameraCache::ALLOCATED_POINTER_ADDRESS) - rip_for_disp);
            if (disp64 < INT32_MIN || disp64 > INT32_MAX)
                throw MemoryUtility::MemoryManipFailedException("RIP-relative displacement too large for car transform pointer save.");

            int32_t disp32 = static_cast<int32_t>(disp64);
            std::memcpy(&trampoline_code[3], &disp32, sizeof(disp32));

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, CameraCache::ALLOCATED_CAVE_ADDRESS, trampoline_code.data(), trampoline_code.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to write trampoline.");

            ////////////////////////////////////////
            // Install Hook - 14 byte absolute jump
            ////////////////////////////////////////
            std::vector<uint8_t> hook_bytes(CameraCache::ORIGINAL_CODE_SIZE, 0x90);
            hook_bytes[0] = 0xFF;
            hook_bytes[1] = 0x25;
            hook_bytes[2] = 0x00;
            hook_bytes[3] = 0x00;
            hook_bytes[4] = 0x00;
            hook_bytes[5] = 0x00;
            std::memcpy(&hook_bytes[6], &CameraCache::ALLOCATED_CAVE_ADDRESS, 8);
            // Bytes 14-15 are NOPs

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, CameraCache::ORIGINAL_CODE_ADDRESS, hook_bytes.data(), CameraCache::ORIGINAL_CODE_SIZE))
                throw MemoryUtility::MemoryManipFailedException("Failed to install hook.");

            hook_installed = true;

            ////////////////////////////////////////
            // Try capture value
            ////////////////////////////////////////
            uintptr_t captured_value = 0;

            CoreEngine::Units::MilliSecond deadline = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;

            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, CameraCache::ALLOCATED_POINTER_ADDRESS, &captured_value, sizeof(captured_value))) 
                {
                    if (captured_value != 0) 
                    {
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            ////////////////////////////////////////
            // Restore code & cleanup
            ////////////////////////////////////////
            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, CameraCache::ORIGINAL_CODE_ADDRESS, CameraCache::ORIGINAL_CODE.data(), CameraCache::ORIGINAL_CODE_SIZE))
            {
                ENGINE_ERROR_PRINT("Warning: Failed to restore original code after timeout.");
            }
            hook_installed = false;

            if (captured_value == 0)
                throw MemoryUtility::MemoryManipFailedException("Failed to capture car transform pointer after waiting.");

            return captured_value;
        }
        catch (...) 
        {
            if (hook_installed) 
            {
                if (! MemoryUtility::TryWriteMemoryOrNothing(&process, CameraCache::ORIGINAL_CODE_ADDRESS, CameraCache::ORIGINAL_CODE.data(), CameraCache::ORIGINAL_CODE_SIZE))
                {
                    ENGINE_ERROR_PRINT("Warning: Failed to restore original code after timeout.");
                }
            }
            
            throw;
        }
    }

    [[deprecated("Use FinalCameraStateAddresses()")]]
    uintptr_t MemoryAddressFinder::FindActionCameraBaseAddress(CoreEngine::Units::MilliSecond max_wait_time)
    {
        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        libmem::Address original = MemoryUtility::AOBScanOrThrow(&process, "F2 0F 11 87 70 FE FF FF 41 0F 28 C3", module.base, module.size);

        constexpr size_t stolen_size = 16;
        std::array<uint8_t, stolen_size> stolen{};
        MemoryUtility::ReadMemoryOrThrow(&process, original, stolen.data(), stolen_size);

        libmem::Address cave_address = MemoryUtility::AllocMemoryOrThrow(&process, 0x1000, libmem::Prot::XRW);
        libmem::Address cam_ptr_addr = MemoryUtility::AllocMemoryOrThrow(&process, 8, libmem::Prot::RW);

        // Shall store return result
        uintptr_t captured_value = 0;
                
        bool hook_installed = false;

        try 
        {
            //////////////////////////////////////////////////////////
            // Build trampoline
            //////////////////////////////////////////////////////////
            std::vector<uint8_t> trampoline;
            trampoline.push_back(0x50); // push rax
            
            // mov rax, <cam_ptr_addr>
            trampoline.insert(trampoline.end(), { 0x48, 0xB8 });
            trampoline.insert(trampoline.end(), (uint8_t*)&cam_ptr_addr, (uint8_t*)&cam_ptr_addr + 8);
            
            // mov [rax], rdi
            trampoline.insert(trampoline.end(), { 0x48, 0x89, 0x38 });
            
            trampoline.push_back(0x58); // pop rax
            
            // Execute stolen bytes
            trampoline.insert(trampoline.end(), stolen.begin(), stolen.end());

            // Absolute 14-byte Jump Back (FF 25 [00000000] <addr>)
            trampoline.insert(trampoline.end(), { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 });
            uint64_t ret_addr = (uint64_t)original + stolen_size;
            trampoline.insert(trampoline.end(), (uint8_t*)&ret_addr, (uint8_t*)&ret_addr + 8);

            MemoryUtility::WriteMemoryOrThrow(&process, cave_address, trampoline.data(), trampoline.size());

            //////////////////////////////////////////////////////////
            // Install hook
            //////////////////////////////////////////////////////////
            std::vector<uint8_t> hook(stolen_size, 0x90); 
            hook[0] = 0xFF; 
            hook[1] = 0x25;
            *(uint32_t*)&hook[2] = 0; 
            std::memcpy(&hook[6], &cave_address, 8);

            MemoryUtility::WriteMemoryOrThrow(&process, original, hook.data(), stolen_size);
            hook_installed = true;

            //////////////////////////////////////////////////////////
            // Try capture value
            //////////////////////////////////////////////////////////
            CoreEngine::Units::MilliSecond deadline  = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;

            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, cam_ptr_addr, &captured_value, 8))
                    if (captured_value != 0)
                        break;

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        catch (...) 
        {
            if (hook_installed)
            {
                MemoryUtility::TryWriteMemoryOrNothing(&process, original, stolen.data(), stolen_size);
            }
            MemoryUtility::TryFreeMemoryOrNothing(&process, cam_ptr_addr, 8);
            throw;
        }

        //////////////////////////////////////////////////////////
        // Cleanup
        //////////////////////////////////////////////////////////
        MemoryUtility::TryWriteMemoryOrNothing(&process, original, stolen.data(), stolen_size);

        // Verify unhook success
        std::array<uint8_t, stolen_size> verify{};
        MemoryUtility::ReadMemoryOrThrow(&process, original, verify.data(), stolen_size);
        
        MemoryUtility::TryFreeMemoryOrNothing(&process, cam_ptr_addr, 8);

        if (verify != stolen)
            throw MemoryUtility::MemoryManipFailedException("Action Camera hook cleanup failed");

        if (captured_value == 0)
            throw MemoryUtility::MemoryManipFailedException("Action Camera capture timed out.");

        return captured_value;
    }

    namespace LapTimeCache
    {
        static std::mutex MUTEX;

        ////////////////////////////////////////
        // Original code
        ////////////////////////////////////////
        static libmem::Address ORIGINAL_CODE_ADDRESS = INVALID_ADDRESS;  // AOB address (INJECT_TIMER, NOT +4)

        // Steal all 3 instructions: add [rdi+10],rax (4) + mov rdx,[rdi+F8] (7) + mov rax,[rdx+08] (4) = 15
        // 14-byte absolute jmp hook + 1 NOP fits exactly
        constexpr static size_t                        ORIGINAL_CODE_SIZE = 15;
        static std::array<uint8_t, ORIGINAL_CODE_SIZE> ORIGINAL_CODE;
        static bool                                    ORIGINAL_CODE_CACHE_HAS_VALUE = false;

        ////////////////////////////////////////
        // Cave
        ////////////////////////////////////////
        constexpr size_t       ALLOCATED_CAVE_SIZE    = 0x1000;
        static libmem::Address ALLOCATED_CAVE_ADDRESS = INVALID_ADDRESS;

        ////////////////////////////////////////
        // Found pointer storage
        ////////////////////////////////////////
        constexpr static size_t ALLOCATED_POINTER_SIZE    = sizeof(uintptr_t);
        static libmem::Address  ALLOCATED_POINTER_ADDRESS = INVALID_ADDRESS;
    }

    uintptr_t MemoryAddressFinder::FindLapTimeAddress(CoreEngine::Units::MilliSecond max_wait_time)
    {
        std::scoped_lock lock(LapTimeCache::MUTEX);

        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        ////////////////////////////////////////
        // Find hook location & save original code
        ////////////////////////////////////////
        if (LapTimeCache::ORIGINAL_CODE_ADDRESS == INVALID_ADDRESS)
        {
            LapTimeCache::ORIGINAL_CODE_ADDRESS = MemoryUtility::AOBScanOrThrow(&process, "48 01 47 10 48 8B 97 F8 00 00 00 48 8B 42 08", module.base, module.size);
        }

        if (! LapTimeCache::ORIGINAL_CODE_CACHE_HAS_VALUE)
        {
            // Steal 15 bytes: add [rdi+10],rax (4) + mov rdx,[rdi+F8] (7) + mov rax,[rdx+08] (4)
            MemoryUtility::ReadMemoryOrThrow(&process, LapTimeCache::ORIGINAL_CODE_ADDRESS, LapTimeCache::ORIGINAL_CODE.data(), LapTimeCache::ORIGINAL_CODE_SIZE);
            LapTimeCache::ORIGINAL_CODE_CACHE_HAS_VALUE = true;
        }

        ////////////////////////////////////////
        // Allocate cave and pointer
        ////////////////////////////////////////
        if (LapTimeCache::ALLOCATED_CAVE_ADDRESS == INVALID_ADDRESS)
        {
            LapTimeCache::ALLOCATED_CAVE_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, LapTimeCache::ALLOCATED_CAVE_SIZE, libmem::Prot::XRW);
        }

        if (LapTimeCache::ALLOCATED_POINTER_ADDRESS == INVALID_ADDRESS)
        {
            LapTimeCache::ALLOCATED_POINTER_ADDRESS = MemoryUtility::AllocMemoryOrThrow(&process, LapTimeCache::ALLOCATED_POINTER_SIZE, libmem::Prot::RW);
        }

        ////////////////////////////////////////
        // Clear previous
        ////////////////////////////////////////
        uintptr_t zero = 0;
        MemoryUtility::WriteMemoryOrThrow(&process, LapTimeCache::ALLOCATED_POINTER_ADDRESS, &zero, sizeof(zero));

        bool hook_installed {false};

        try
        {
            ////////////////////////////////////////
            // Build trampoline
            //
            // Layout (all offsets from cave base):
            //   [0 ] add [rdi+10], rax          — 4 bytes  (stolen instr 1, execute first)
            //   [4 ] mov [rip+disp32], rdi       — 7 bytes  (save rdi; RIP-after = cave+11)
            //   [11] mov rdx,[rdi+000000F8]      — 7 bytes  (stolen instr 2)
            //   [18] mov rax,[rdx+08]            — 4 bytes  (stolen instr 3)
            //   [22] FF 25 00 00 00 00           — 6 bytes  (indirect jmp)
            //   [28] <8-byte return address>     — 8 bytes
            ////////////////////////////////////////
            std::vector<uint8_t> trampoline_code;
            trampoline_code.reserve(36);

            // Execute stolen instruction 1: add [rdi+10], rax  (bytes 0..3)
            trampoline_code.insert(trampoline_code.end(), {0x48, 0x01, 0x47, 0x10});

            // mov [rip+disp32], rdi  (bytes 4..10); disp patched below, RIP-after = cave+11
            trampoline_code.insert(trampoline_code.end(), {0x48, 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00});

            // Execute stolen instructions 2 & 3  (bytes 11..21)
            trampoline_code.insert(trampoline_code.end(),
                LapTimeCache::ORIGINAL_CODE.begin() + 4,   // skip the already-executed add
                LapTimeCache::ORIGINAL_CODE.end());

            // Absolute indirect jmp back  (bytes 22..35)
            trampoline_code.insert(trampoline_code.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
            uint64_t ret_addr = static_cast<uint64_t>(LapTimeCache::ORIGINAL_CODE_ADDRESS + LapTimeCache::ORIGINAL_CODE_SIZE);
            trampoline_code.insert(trampoline_code.end(),
                reinterpret_cast<const uint8_t*>(&ret_addr),
                reinterpret_cast<const uint8_t*>(&ret_addr) + 8);

            // Patch RIP-relative disp for: mov [rip+disp], rdi
            // Instruction is at cave+4, length 7, so RIP-after = cave+11
            uint64_t rip_for_disp = static_cast<uint64_t>(LapTimeCache::ALLOCATED_CAVE_ADDRESS) + 11;
            int64_t disp64 = static_cast<int64_t>(static_cast<uint64_t>(LapTimeCache::ALLOCATED_POINTER_ADDRESS) - rip_for_disp);
            if (disp64 < INT32_MIN || disp64 > INT32_MAX)
                throw MemoryUtility::MemoryManipFailedException("RIP-relative displacement too large for lap time rdi save.");

            int32_t disp32 = static_cast<int32_t>(disp64);
            std::memcpy(&trampoline_code[7], &disp32, sizeof(disp32));  // disp field is at offset 4+3=7

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, LapTimeCache::ALLOCATED_CAVE_ADDRESS, trampoline_code.data(), trampoline_code.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to write trampoline.");

            ////////////////////////////////////////
            // Install Hook — 14-byte absolute indirect jmp + 1 NOP (fits 15 stolen bytes)
            ////////////////////////////////////////
            std::vector<uint8_t> hook_bytes(LapTimeCache::ORIGINAL_CODE_SIZE, 0x90);
            hook_bytes[0] = 0xFF;
            hook_bytes[1] = 0x25;
            hook_bytes[2] = 0x00;
            hook_bytes[3] = 0x00;
            hook_bytes[4] = 0x00;
            hook_bytes[5] = 0x00;
            std::memcpy(&hook_bytes[6], &LapTimeCache::ALLOCATED_CAVE_ADDRESS, 8);
            // hook_bytes[14] = 0x90 (NOP, already set)

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, LapTimeCache::ORIGINAL_CODE_ADDRESS, hook_bytes.data(), LapTimeCache::ORIGINAL_CODE_SIZE))
                throw MemoryUtility::MemoryManipFailedException("Failed to install hook.");

            hook_installed = true;

            ////////////////////////////////////////
            // Try capture value
            ////////////////////////////////////////
            uintptr_t captured_value = 0;

            CoreEngine::Units::MilliSecond deadline = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;
            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, LapTimeCache::ALLOCATED_POINTER_ADDRESS, &captured_value, sizeof(captured_value)))
                {
                    if (captured_value != 0)
                        break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            ////////////////////////////////////////
            // Restore code & cleanup
            ////////////////////////////////////////
            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, LapTimeCache::ORIGINAL_CODE_ADDRESS, LapTimeCache::ORIGINAL_CODE.data(), LapTimeCache::ORIGINAL_CODE_SIZE))
            {
                ENGINE_ERROR_PRINT("Warning: Failed to restore original code after lap time capture.");
            }
            hook_installed = false;

            if (captured_value == 0)
                throw MemoryUtility::MemoryManipFailedException("Failed to capture lap time base pointer after waiting.");

            return captured_value + 0x10;
        }
        catch (...)
        {
            if (hook_installed)
            {
                if (! MemoryUtility::TryWriteMemoryOrNothing(&process, LapTimeCache::ORIGINAL_CODE_ADDRESS, LapTimeCache::ORIGINAL_CODE.data(), LapTimeCache::ORIGINAL_CODE_SIZE))
                {
                    ENGINE_ERROR_PRINT("Warning: Failed to restore original code after lap time exception.");
                }
            }

            throw;
        }
    }

    namespace ProgressCache
    {
        static std::mutex MUTEX;

        static libmem::Address ORIGINAL_CODE_ADDRESS = INVALID_ADDRESS;

        constexpr static size_t                        ORIGINAL_CODE_SIZE = 6;
        static std::array<uint8_t, ORIGINAL_CODE_SIZE> ORIGINAL_CODE;
        static bool                                    ORIGINAL_CODE_CACHE_HAS_VALUE = false;

        constexpr size_t       ALLOCATED_CAVE_SIZE    = 0x1000;
        static libmem::Address ALLOCATED_CAVE_ADDRESS = INVALID_ADDRESS;

        // Pointer storage lives inside the cave always within ±2 GB of the trampoline
        constexpr static size_t POINTER_OFFSET_IN_CAVE = 0x100;
    }

    uintptr_t MemoryAddressFinder::FindRaceProgressAddress(CoreEngine::Units::MilliSecond max_wait_time)
    {
        std::scoped_lock lock(ProgressCache::MUTEX);

        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        if (ProgressCache::ORIGINAL_CODE_ADDRESS == INVALID_ADDRESS)
        {
            ProgressCache::ORIGINAL_CODE_ADDRESS = MemoryUtility::AOBScanOrThrow(&process, "89 87 D8 01 00 00 48 83 C4 38", module.base, module.size);
        }

        if (! ProgressCache::ORIGINAL_CODE_CACHE_HAS_VALUE)
        {
            MemoryUtility::ReadMemoryOrThrow(&process, ProgressCache::ORIGINAL_CODE_ADDRESS, ProgressCache::ORIGINAL_CODE.data(), ProgressCache::ORIGINAL_CODE_SIZE);
            ProgressCache::ORIGINAL_CODE_CACHE_HAS_VALUE = true;
        }

        if (ProgressCache::ALLOCATED_CAVE_ADDRESS == INVALID_ADDRESS)
        {
            ProgressCache::ALLOCATED_CAVE_ADDRESS = 
            MemoryUtility::AllocMemoryNearAddressOrThrow(&process, static_cast<uintptr_t>(ProgressCache::ORIGINAL_CODE_ADDRESS), ProgressCache::ALLOCATED_CAVE_SIZE, 0x70000000);
        }

        libmem::Address pointerAddress = ProgressCache::ALLOCATED_CAVE_ADDRESS + ProgressCache::POINTER_OFFSET_IN_CAVE;

        uintptr_t zero = 0;
        MemoryUtility::WriteMemoryOrThrow(&process, pointerAddress, &zero, sizeof(zero));

        bool hook_installed {false};

        try
        {
            ////////////////////////////////////////
            // Build trampoline
            //
            //   [0 ] mov [rdi+1D8],eax      —  6 bytes  (original instruction)
            //   [6 ] mov [rip+disp32],rdi   —  7 bytes  (save rdi; RIP-after = cave+13)
            //   [13] FF 25 00 00 00 00      —  6 bytes  (indirect absolute jmp back)
            //   [19] <8-byte return addr>   —  8 bytes
            ////////////////////////////////////////
            std::vector<uint8_t> trampoline_code;
            trampoline_code.reserve(27);

            // Original instruction: mov [rdi+1D8],eax
            trampoline_code.insert(trampoline_code.end(), {0x89, 0x87, 0xD8, 0x01, 0x00, 0x00});

            // mov [rip+disp32],rdi — save rdi; disp patched below, RIP-after = cave+13
            trampoline_code.insert(trampoline_code.end(), {0x48, 0x89, 0x3D, 0x00, 0x00, 0x00, 0x00});

            // Absolute indirect jmp back
            trampoline_code.insert(trampoline_code.end(), {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
            uint64_t ret_addr = static_cast<uint64_t>(ProgressCache::ORIGINAL_CODE_ADDRESS + ProgressCache::ORIGINAL_CODE_SIZE);
            trampoline_code.insert(trampoline_code.end(),
                reinterpret_cast<const uint8_t*>(&ret_addr),
                reinterpret_cast<const uint8_t*>(&ret_addr) + 8);

            // Patch disp: instruction at cave+6, length 7, RIP-after = cave+13
            // target = cave+0x100 — same allocation, trivially fits
            uint64_t rip_after = static_cast<uint64_t>(ProgressCache::ALLOCATED_CAVE_ADDRESS) + 13;
            int64_t  disp64    = static_cast<int64_t>(static_cast<uint64_t>(pointerAddress) - rip_after);
            if (disp64 < INT32_MIN || disp64 > INT32_MAX)
                throw MemoryUtility::MemoryManipFailedException("Pointer offset within cave overflowed — should never happen.");

            int32_t disp32 = static_cast<int32_t>(disp64);
            std::memcpy(&trampoline_code[9], &disp32, sizeof(disp32)); 

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, ProgressCache::ALLOCATED_CAVE_ADDRESS, trampoline_code.data(), trampoline_code.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to write progress trampoline.");

            ////////////////////////////////////////
            // Install Hook — E9 rel32 (5 bytes) + NOP (1 byte)
            ////////////////////////////////////////
            std::vector<uint8_t> hook_bytes(ProgressCache::ORIGINAL_CODE_SIZE, 0x90);
            hook_bytes[0] = 0xE9;

            int64_t jmp_disp64 = static_cast<int64_t>(static_cast<uint64_t>(ProgressCache::ALLOCATED_CAVE_ADDRESS))
                            - static_cast<int64_t>(static_cast<uint64_t>(ProgressCache::ORIGINAL_CODE_ADDRESS) + 5);
            if (jmp_disp64 < INT32_MIN || jmp_disp64 > INT32_MAX)
                throw MemoryUtility::MemoryManipFailedException("Cave outside E9 range — AllocMemoryNearAddress did not place it close enough.");

            int32_t jmp_disp32 = static_cast<int32_t>(jmp_disp64);
            std::memcpy(&hook_bytes[1], &jmp_disp32, sizeof(jmp_disp32));

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, ProgressCache::ORIGINAL_CODE_ADDRESS, hook_bytes.data(), ProgressCache::ORIGINAL_CODE_SIZE))
                throw MemoryUtility::MemoryManipFailedException("Failed to install progress hook.");

            hook_installed = true;

            ////////////////////////////////////////
            // Try capture value
            ////////////////////////////////////////
            uintptr_t captured_value = 0;
            CoreEngine::Units::MilliSecond deadline = CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() + max_wait_time;
            while (CoreEngine::Timer::GetMonotonicTime<CoreEngine::Units::MilliSecond>() < deadline)
            {
                if (MemoryUtility::TryReadMemoryOrNothing(&process, pointerAddress, &captured_value, sizeof(captured_value)))
                {
                    if (captured_value != 0)
                        break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (! MemoryUtility::TryWriteMemoryOrNothing(&process, ProgressCache::ORIGINAL_CODE_ADDRESS, ProgressCache::ORIGINAL_CODE.data(), ProgressCache::ORIGINAL_CODE_SIZE))
                throw MemoryUtility::MemoryManipFailedException("Failed to restore original progress code - hook bytes left in game.");

            hook_installed = false;

            if (captured_value == 0)
                throw MemoryUtility::MemoryManipFailedException("Failed to capture progress address after waiting.");

            return captured_value + 0x1D8;
        }
        catch (...)
        {
            if (hook_installed)
            {
                if (! MemoryUtility::TryWriteMemoryOrNothing(&process, ProgressCache::ORIGINAL_CODE_ADDRESS, ProgressCache::ORIGINAL_CODE.data(), ProgressCache::ORIGINAL_CODE_SIZE))
                    ENGINE_ERROR_PRINT("Warning: Failed to restore original progress code after exception.");
            }
            throw;
        }
    }

    void MemoryAddressFinder::InvalidateCache() noexcept
    {
        std::scoped_lock lock(RacerCache::MUTEX, CameraCache::MUTEX, LapTimeCache::MUTEX, ProgressCache::MUTEX);
        try 
        {
            const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

            auto FreeIfRequired = [&process](libmem::Address address, size_t size) -> void
            {
                if (address != INVALID_ADDRESS)
                {
                    MemoryUtility::TryFreeMemoryOrNothing(&process, address, size);
                }
            };

            FreeIfRequired(RacerCache::ALLOCATED_CAVE_ADDRESS,    RacerCache::ALLOCATED_CAVE_SIZE);
            FreeIfRequired(RacerCache::ALLOCATED_POINTER_ADDRESS, RacerCache::ALLOCATED_POINTER_SIZE);

            FreeIfRequired(CameraCache::ALLOCATED_CAVE_ADDRESS,    CameraCache::ALLOCATED_CAVE_SIZE);
            FreeIfRequired(CameraCache::ALLOCATED_POINTER_ADDRESS, CameraCache::ALLOCATED_POINTER_SIZE);

            FreeIfRequired(LapTimeCache::ALLOCATED_CAVE_ADDRESS,    LapTimeCache::ALLOCATED_CAVE_SIZE);
            FreeIfRequired(LapTimeCache::ALLOCATED_POINTER_ADDRESS, LapTimeCache::ALLOCATED_POINTER_SIZE);

            if (ProgressCache::ALLOCATED_CAVE_ADDRESS != INVALID_ADDRESS)
            {
                MemoryUtility::FreeMemoryAllocatedNearAddressOrThrow(&process, ProgressCache::ALLOCATED_CAVE_ADDRESS, ProgressCache::ALLOCATED_CAVE_SIZE);
            }
        }  
        catch (const std::exception& e)
        {
            ENGINE_ERROR_PRINT("MemoryAddressFinder: Failed to free memory: " << e.what());
        }

        RacerCache::ORIGINAL_CODE_ADDRESS            = INVALID_ADDRESS;
        RacerCache::ORIGINAL_CODE_FOR_OFFSET_ADDRESS = INVALID_ADDRESS;
        RacerCache::ORIGINAL_CODE_CACHE_HAS_VALUE    = false;
        RacerCache::ALLOCATED_CAVE_ADDRESS           = INVALID_ADDRESS;
        RacerCache::ALLOCATED_POINTER_ADDRESS        = INVALID_ADDRESS;


        CameraCache::ORIGINAL_CODE_ADDRESS         = INVALID_ADDRESS;
        CameraCache::ORIGINAL_CODE_CACHE_HAS_VALUE = false;
        CameraCache::ALLOCATED_CAVE_ADDRESS        = INVALID_ADDRESS;
        CameraCache::ALLOCATED_POINTER_ADDRESS     = INVALID_ADDRESS;

        LapTimeCache::ORIGINAL_CODE_ADDRESS         = INVALID_ADDRESS;
        LapTimeCache::ORIGINAL_CODE_CACHE_HAS_VALUE = false;
        LapTimeCache::ALLOCATED_CAVE_ADDRESS        = INVALID_ADDRESS;
        LapTimeCache::ALLOCATED_POINTER_ADDRESS     = INVALID_ADDRESS;

        ProgressCache::ORIGINAL_CODE_ADDRESS         = INVALID_ADDRESS;
        ProgressCache::ORIGINAL_CODE_CACHE_HAS_VALUE = false;
        ProgressCache::ALLOCATED_CAVE_ADDRESS        = INVALID_ADDRESS;
    }
}