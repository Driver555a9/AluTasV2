#include "tas/memory/MemoryRW.h"

#include "tas/globalstate/GameState.h"
#include "tas/globalstate/MemoryAddressState.h"
#include "tas/memory/MemoryUtility.h"

#include "libmem/libmem.hpp"

#include <utility>
#include <mutex>

namespace AsphaltTas
{
    //////////////////////////////////////////////////////////
    // Read
    //////////////////////////////////////////////////////////
    RacerState MemoryRW::ReadRacerState()
    {
        if (! RacerStateAddresses::AddressesAreValid()) 
            throw MemoryUtility::MemoryManipFailedException("ReadRacerState(): Requires valid RacerStateAddresses to read RacerState.");

        const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

        glm::mat4 trans;
        glm::vec3 velocity;

        // Reading from begin of transform matrix, up to + including velocity vec3
        constexpr size_t RACER_BLOCK_SIZE = RacerStateAddresses::GetByteSizeBaseToLastElementInclusive();

        std::array<std::byte, RACER_BLOCK_SIZE> buffer;

        MemoryUtility::ReadMemoryOrThrow(&process, RacerStateAddresses::GetBaseAddress(), buffer.data(), sizeof(buffer));

        std::memcpy(glm::value_ptr(trans),    &buffer[RacerStateAddresses::OFFSET_TRANS_MATRIX4x4], sizeof(decltype(trans)));
        std::memcpy(glm::value_ptr(velocity), &buffer[RacerStateAddresses::OFFSET_VELOCITY_VEC3], sizeof(decltype(velocity)));

        //////////////////////////////////////////////////////////
        // Swapping Y and Z to convert to XYZ convention; Flipping sign of z for handiness
        //////////////////////////////////////////////////////////
        std::swap(velocity.y, velocity.z);
        velocity.z *= -1.0f;

        return RacerState {trans, velocity};
    }

    CameraState MemoryRW::ReadCameraState()
    {
        if (! CameraStateAddresses::AddressesAreValid()) 
            throw MemoryUtility::MemoryManipFailedException("ReadCameraState(): Requires valid CameraStateAddresses to read CameraState.");

        const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

        constexpr size_t CAMERA_BLOCK_SIZE = CameraStateAddresses::GetByteSizeBaseToLastElementInclusive();

        std::array<std::byte, CAMERA_BLOCK_SIZE> buffer;

        MemoryUtility::ReadMemoryOrThrow(&process, CameraStateAddresses::GetBaseAddress(), buffer.data(), sizeof(buffer));

        CameraState game_state;

        std::memcpy(glm::value_ptr(game_state.m_position),  &buffer[CameraStateAddresses::OFFSET_POSITON_VEC3],  sizeof(decltype(game_state.m_position)));
        std::memcpy(glm::value_ptr(game_state.m_rotation),  &buffer[CameraStateAddresses::OFFSET_ROTATION_QUAT], sizeof(decltype(game_state.m_rotation)));
        std::memcpy(&game_state.m_fov_radians,              &buffer[CameraStateAddresses::OFFSET_FOV_RADIANS],   sizeof(decltype(game_state.m_fov_radians)));
        std::memcpy(&game_state.m_aspect_ratio,             &buffer[CameraStateAddresses::OFFSET_ASPECT_RATIO],  sizeof(decltype(game_state.m_aspect_ratio)));

        //////////////////////////////////////////////////////////
        // Swapping Y and Z to convert to XYZ convention; Flipping sign of z for handiness
        //////////////////////////////////////////////////////////
        std::swap(game_state.m_position.y, game_state.m_position.z);
        game_state.m_position.z *= -1.0f;

        std::swap(game_state.m_rotation.y, game_state.m_rotation.z);
        game_state.m_rotation.z *= -1.0f;

        return game_state;
    }
        
    //////////////////////////////////////////////////////////
    // Write
    //////////////////////////////////////////////////////////
    void MemoryRW::WriteRacerState(const RacerState& state)
    {
        if (! RacerStateAddresses::AddressesAreValid()) 
            throw MemoryUtility::MemoryManipFailedException("WriteRacerState(): Requires valid RacerStateAddresses to write RacerState.");

        const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

        //////////////////////////////////////////////////////////
        // Swapping Y and Z to convert to XZY convention; Flipping sign of Z in for handiness
        //////////////////////////////////////////////////////////
        glm::vec3 xyz_velocity = state.GetVelocity();
        xyz_velocity.z *= -1.0f;
        std::swap(xyz_velocity.y, xyz_velocity.z);

        MemoryUtility::WriteGlmValueOrThrow(&process, RacerStateAddresses::GetTransMatrixAddress(),  state.GetGameConventionTransformMatrix());
        MemoryUtility::WriteGlmValueOrThrow(&process, RacerStateAddresses::GetVelocityVec3Address(), xyz_velocity);
    }

    void MemoryRW::WriteCameraState(const CameraState& state, IGNORE_FLAG_CAMERA ignore_flags)
    {
        if (! CameraStateAddresses::AddressesAreValid()) 
            throw MemoryUtility::MemoryManipFailedException("FinalCameraStateAddresses(): Requires valid CameraStateAddresses to write CameraState.");

        const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

        CameraState copy = state;
        
        //////////////////////////////////////////////////////////
        // Swapping Y and Z to convert to XZY convention; Flipping sign of Z in for handiness
        //////////////////////////////////////////////////////////
        copy.m_position.z *= -1.0f;
        std::swap(copy.m_position.y, copy.m_position.z);

        copy.m_rotation.z *= -1.0f;
        std::swap(copy.m_rotation.y, copy.m_rotation.z);

        //////////////////////////////////////////////////////////
        // Read current data -> then write (1Read, 1Write) better than 4 Writes
        //////////////////////////////////////////////////////////
        constexpr size_t CAMERA_BLOCK_SIZE = CameraStateAddresses::GetByteSizeBaseToLastElementInclusive();

        std::array<std::byte, CAMERA_BLOCK_SIZE> buffer;

        MemoryUtility::ReadMemoryOrThrow(&process, CameraStateAddresses::GetBaseAddress(), buffer.data(), sizeof(buffer));

        if ((ignore_flags & IGNORE_FLAG_CAMERA::PositionVec3) == IGNORE_FLAG_CAMERA::NONE)
            std::memcpy(&buffer[CameraStateAddresses::OFFSET_POSITON_VEC3], glm::value_ptr(copy.m_position), sizeof(decltype(copy.m_position)));

        if ((ignore_flags & IGNORE_FLAG_CAMERA::RotationQuat) == IGNORE_FLAG_CAMERA::NONE)
            std::memcpy(&buffer[CameraStateAddresses::OFFSET_ROTATION_QUAT], glm::value_ptr(copy.m_rotation), sizeof(decltype(copy.m_rotation)));

        if ((ignore_flags & IGNORE_FLAG_CAMERA::FovFloat) == IGNORE_FLAG_CAMERA::NONE)
            std::memcpy(&buffer[CameraStateAddresses::OFFSET_FOV_RADIANS], &copy.m_fov_radians, sizeof(decltype(copy.m_fov_radians)));
        
        if ((ignore_flags & IGNORE_FLAG_CAMERA::AspectRatio) == IGNORE_FLAG_CAMERA::NONE)
            std::memcpy(&buffer[CameraStateAddresses::OFFSET_ASPECT_RATIO], &copy.m_aspect_ratio, sizeof(decltype(copy.m_aspect_ratio)));


        MemoryUtility::WriteMemoryOrThrow(&process, CameraStateAddresses::GetBaseAddress(), buffer.data(), sizeof(buffer));
    }

//////////////////////////////////////////////////////////
// Camera manipulation
//////////////////////////////////////////////////////////
    namespace CameraDestroyCache
    {
        static std::atomic<bool> CAMERA_CODE_IS_DESTROYED = false;
        static std::mutex DESTROY_RESTORE_CAMERA_MUTEX;

        //////////////////////////////////////////////////////////
        // Position 
        //////////////////////////////////////////////////////////
        static std::array<uint8_t, 5> ORIGINAL_POSITION_XZ;  // movsd [rcx+38],xmm0
        static std::array<uint8_t, 3> ORIGINAL_POSITION_Y;   // mov [rcx+40],eax

        static libmem::Address POSITION_XY_ADDRESS = INVALID_ADDRESS;
        static libmem::Address POSITION_Z_ADDRESS  = INVALID_ADDRESS;

        static libmem::Address POSITION_BASE_ADDRESS_CACHE = INVALID_ADDRESS;

        //////////////////////////////////////////////////////////
        // Rotation
        //////////////////////////////////////////////////////////
        static std::array<uint8_t, 5> ORIGINAL_ROTATION_X;   // movss [rcx+44],xmm1
        static std::array<uint8_t, 3> ORIGINAL_ROTATION_Z;   // mov [rcx+48],eax
        static std::array<uint8_t, 3> ORIGINAL_ROTATION_Y;   // mov [rcx+4C],eax
        static std::array<uint8_t, 3> ORIGINAL_ROTATION_W;   // mov [rcx+50],eax

        static libmem::Address ROTATION_X_ADDRESS  = INVALID_ADDRESS;
        static libmem::Address ROTATION_Z_ADDRESS  = INVALID_ADDRESS;
        static libmem::Address ROTATION_Y_ADDRESS  = INVALID_ADDRESS;
        static libmem::Address ROTATION_W_ADDRESS  = INVALID_ADDRESS;

        static libmem::Address ROTATION_BASE_ADDRESS_CACHE = INVALID_ADDRESS;

        //////////////////////////////////////////////////////////
        // FOV
        //////////////////////////////////////////////////////////
        static std::array<uint8_t, 8> ORIGINAL_FOV;

        static libmem::Address FOV_BASE_ADDRESS_CACHE = INVALID_ADDRESS;

        static libmem::Address FOV_ADDRESS = INVALID_ADDRESS;
    }

    namespace CamCache = CameraDestroyCache;

    void MemoryRW::DestroyCameraUpdateCode()
    {
        std::scoped_lock lock(CamCache::DESTROY_RESTORE_CAMERA_MUTEX);
        
        if (CamCache::CAMERA_CODE_IS_DESTROYED) return;

        auto [process, module] = MemoryUtility::GetAsphaltProcessAndModuleOrThrow();

        try
        {
        //////////////////////////////////////////////////////////
        // Position update code
        //////////////////////////////////////////////////////////
            {
                if (CamCache::POSITION_BASE_ADDRESS_CACHE == INVALID_ADDRESS)
                {
                    CamCache::POSITION_BASE_ADDRESS_CACHE = 
                    MemoryUtility::AOBScanOrThrow(&process, "F2 0F 10 02 F2 0F 11 41 38 8B 42 08 89 41 40 C6 41 58 01", module.base, module.size);
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // Offsets from pattern start:
                // +0:  F2 0F 10 02        (movsd xmm0,[rdx])       - 4 bytes
                // +4:  F2 0F 11 41 38     (movsd [rcx+38],xmm0)    - 5 bytes - NOP THIS (X, Z)
                // +9:  8B 42 08           (mov eax,[rdx+08])       - 3 bytes
                // +12: 89 41 40           (mov [rcx+40],eax)       - 3 bytes - NOP THIS (Y)
                // +15: C6 41 58 01        (mov byte ptr [rcx+58],01)
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                CamCache::POSITION_XY_ADDRESS = CamCache::POSITION_BASE_ADDRESS_CACHE + 4;   // movsd [rcx+38]
                CamCache::POSITION_Z_ADDRESS  = CamCache::POSITION_BASE_ADDRESS_CACHE + 12;  // mov   [rcx+40]
            }

        //////////////////////////////////////////////////////////
        // Rotation update code
        //////////////////////////////////////////////////////////
            {
                if (CameraDestroyCache::ROTATION_BASE_ADDRESS_CACHE == INVALID_ADDRESS)
                {
                    CameraDestroyCache::ROTATION_BASE_ADDRESS_CACHE 
                    = MemoryUtility::AOBScanOrThrow(&process, "F3 0F 11 49 44 8B 42 04 89 41 48 8B 42 08 89 41 4C 8B 42 0C 89 41 50", module.base, module.size);
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // Offsets from pattern start:
                // +0:  F3 0F 11 49 44     (movss [rcx+44],xmm1)    - 5 bytes - NOP this (X)
                // +5:  8B 42 04           (mov eax,[rdx+04])       - 3 bytes
                // +8:  89 41 48           (mov [rcx+48],eax)       - 3 bytes - NOP this (Z)
                // +11: 8B 42 08           (mov eax,[rdx+08])       - 3 bytes
                // +14: 89 41 4C           (mov [rcx+4C],eax)       - 3 bytes - NOP this (Y)
                // +17: 8B 42 0C           (mov eax,[rdx+0C])       - 3 bytes
                // +20: 89 41 50           (mov [rcx+50],eax)       - 3 bytes - NOP this (W)
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                CamCache::ROTATION_X_ADDRESS = CamCache::ROTATION_BASE_ADDRESS_CACHE;      // movss [rcx+44]
                CamCache::ROTATION_Z_ADDRESS = CamCache::ROTATION_BASE_ADDRESS_CACHE + 8;  // mov [rcx+48]
                CamCache::ROTATION_Y_ADDRESS = CamCache::ROTATION_BASE_ADDRESS_CACHE + 14; // mov [rcx+4C]
                CamCache::ROTATION_W_ADDRESS = CamCache::ROTATION_BASE_ADDRESS_CACHE + 20; // mov [rcx+50]
           }

        //////////////////////////////////////////////////////////
        // FOV Code
        //////////////////////////////////////////////////////////
            {
                if (CamCache::FOV_BASE_ADDRESS_CACHE == INVALID_ADDRESS)
                {
                    CamCache::FOV_BASE_ADDRESS_CACHE = MemoryUtility::AOBScanOrThrow(&process, 
                        "0F 2E 81 28 01 00 00 ?? ?? 39 81 2C 01 00 00 ?? ?? F3 0F 11 81 28 01 00 00 89 81 2C 01 00 00", module.base, module.size);
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // +0 : 0F 2E 81 28 01 00 00      (ucomiss xmm0,[rcx+00000128])
                // +7 : 75 08                     (jne Asphalt9_Steam_x64_rtl.exe+66A56C)
                // +9 : 39 81 2C 01 00 00         ([rcx+0000012C],eax)
                // +15: 74 13                     (je Asphalt9_Steam_x64_rtl.exe+66A57F)
                // +17: F3 0F 11 81 28 01 00 00   (movss [rcx+00000128],xmm0)              - 8 bytes - NOP this
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                CamCache::FOV_ADDRESS = CamCache::FOV_BASE_ADDRESS_CACHE + 17; //movss [rcx+00000128],xmm0
            }

        //////////////////////////////////////////////////////////
        // Save original instructions
        //////////////////////////////////////////////////////////
            // Position
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::POSITION_XY_ADDRESS, CamCache::ORIGINAL_POSITION_XZ.data(), CamCache::ORIGINAL_POSITION_XZ.size());
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::POSITION_Z_ADDRESS,  CamCache::ORIGINAL_POSITION_Y.data(),  CamCache::ORIGINAL_POSITION_Y.size());

            // ROtation
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::ROTATION_X_ADDRESS,  CamCache::ORIGINAL_ROTATION_X.data(),  CamCache::ORIGINAL_ROTATION_X.size());
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::ROTATION_Z_ADDRESS,  CamCache::ORIGINAL_ROTATION_Z.data(),  CamCache::ORIGINAL_ROTATION_Z.size());
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::ROTATION_Y_ADDRESS,  CamCache::ORIGINAL_ROTATION_Y.data(),  CamCache::ORIGINAL_ROTATION_Y.size());
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::ROTATION_W_ADDRESS,  CamCache::ORIGINAL_ROTATION_W.data(),  CamCache::ORIGINAL_ROTATION_W.size());

            // FOV
            MemoryUtility::ReadMemoryOrThrow(&process, CamCache::FOV_ADDRESS, CamCache::ORIGINAL_FOV.data(), CamCache::ORIGINAL_FOV.size());

        //////////////////////////////////////////////////////////
        // NOP original code
        //////////////////////////////////////////////////////////
            std::array<uint8_t, 8> nops_8 = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
            std::array<uint8_t, 5> nops_5 = {0x90, 0x90, 0x90, 0x90, 0x90};
            std::array<uint8_t, 3> nops_3 = {0x90, 0x90, 0x90};

            // Position
            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::POSITION_XY_ADDRESS, nops_5.data(), nops_5.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP position XZ update.");
            
            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::POSITION_Z_ADDRESS, nops_3.data(), nops_3.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP position Y update.");
            
            // Rotation
            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_X_ADDRESS, nops_5.data(), nops_5.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP rotation X update.");
            
            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Z_ADDRESS, nops_3.data(), nops_3.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP rotation Z update.");

            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Y_ADDRESS, nops_3.data(), nops_3.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP rotation Y update.");

            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_W_ADDRESS, nops_3.data(), nops_3.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP rotation W update.");

            // FOV
            if (!MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::FOV_ADDRESS, nops_8.data(), nops_8.size()))
                throw MemoryUtility::MemoryManipFailedException("Failed to NOP FOV update.");

            CamCache::CAMERA_CODE_IS_DESTROYED = true;
        }
        catch(...)
        {
            // Try restore position
            if (CamCache::POSITION_XY_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::POSITION_XY_ADDRESS, CamCache::ORIGINAL_POSITION_XZ.data(), CamCache::ORIGINAL_POSITION_XZ.size());
            if (CamCache::POSITION_Z_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process,CamCache:: POSITION_Z_ADDRESS, CamCache::ORIGINAL_POSITION_Y.data(), CamCache::ORIGINAL_POSITION_Y.size());

            // Try restore rotation
            if (CamCache::ROTATION_X_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_X_ADDRESS, CamCache::ORIGINAL_ROTATION_X.data(), CamCache::ORIGINAL_ROTATION_X.size());
            if (CamCache::ROTATION_Z_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Z_ADDRESS, CamCache::ORIGINAL_ROTATION_Z.data(), CamCache::ORIGINAL_ROTATION_Z.size());
            if (CamCache::ROTATION_Y_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Y_ADDRESS, CamCache::ORIGINAL_ROTATION_Y.data(), CamCache::ORIGINAL_ROTATION_Y.size());
            if (CamCache::ROTATION_W_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_W_ADDRESS, CamCache::ORIGINAL_ROTATION_W.data(), CamCache::ORIGINAL_ROTATION_W.size());

            // Try restore FOV
            if (CamCache::FOV_ADDRESS != 0)
                MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::FOV_ADDRESS, CamCache::ORIGINAL_FOV.data(), CamCache::ORIGINAL_FOV.size());
            throw;
        }
    }

    void MemoryRW::RestoreCameraUpdateCode()
    {
        std::scoped_lock lock(CamCache::DESTROY_RESTORE_CAMERA_MUTEX);
        if (! CamCache::CAMERA_CODE_IS_DESTROYED) return;

        const libmem::Process process = MemoryUtility::GetAsphaltProcessOrThrow();

        // Restore position
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::POSITION_XY_ADDRESS, CamCache::ORIGINAL_POSITION_XZ.data(), CamCache::ORIGINAL_POSITION_XZ.size());
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::POSITION_Z_ADDRESS,  CamCache::ORIGINAL_POSITION_Y.data(),  CamCache::ORIGINAL_POSITION_Y.size() );

        // Restore rotation
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_X_ADDRESS,  CamCache::ORIGINAL_ROTATION_X.data(),  CamCache::ORIGINAL_ROTATION_X.size() );
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Z_ADDRESS,  CamCache::ORIGINAL_ROTATION_Z.data(),  CamCache::ORIGINAL_ROTATION_Z.size() );
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_Y_ADDRESS,  CamCache::ORIGINAL_ROTATION_Y.data(),  CamCache::ORIGINAL_ROTATION_Y.size() );
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::ROTATION_W_ADDRESS,  CamCache::ORIGINAL_ROTATION_W.data(),  CamCache::ORIGINAL_ROTATION_W.size() );

        // Restore FOV
        MemoryUtility::TryWriteMemoryOrNothing(&process, CamCache::FOV_ADDRESS, CamCache::ORIGINAL_FOV.data(), CamCache::ORIGINAL_FOV.size());

        CamCache::CAMERA_CODE_IS_DESTROYED.store(false, std::memory_order::release);
    }

    bool MemoryRW::CameraCodeIsDestroyed() noexcept
    {
        return CamCache::CAMERA_CODE_IS_DESTROYED.load(std::memory_order::acquire);
    }

    void MemoryRW::InvalidateCache() noexcept
    {
        std::scoped_lock lock(CamCache::DESTROY_RESTORE_CAMERA_MUTEX);

        CamCache::CAMERA_CODE_IS_DESTROYED = false;

        //////////////////////////////////////////////////////////
        // Position
        //////////////////////////////////////////////////////////
        CamCache::POSITION_XY_ADDRESS = INVALID_ADDRESS;
        CamCache::POSITION_Z_ADDRESS  = INVALID_ADDRESS;
        CamCache::POSITION_BASE_ADDRESS_CACHE = INVALID_ADDRESS;

        //////////////////////////////////////////////////////////
        // Rotation
        //////////////////////////////////////////////////////////
        CamCache::ROTATION_X_ADDRESS  = INVALID_ADDRESS;
        CamCache::ROTATION_Z_ADDRESS  = INVALID_ADDRESS;
        CamCache::ROTATION_Y_ADDRESS  = INVALID_ADDRESS;
        CamCache::ROTATION_W_ADDRESS  = INVALID_ADDRESS;
        CamCache::ROTATION_BASE_ADDRESS_CACHE = INVALID_ADDRESS;

        //////////////////////////////////////////////////////////
        // FOV
        //////////////////////////////////////////////////////////
        CamCache::FOV_BASE_ADDRESS_CACHE = INVALID_ADDRESS;
        CamCache::FOV_ADDRESS = INVALID_ADDRESS;
    }
}