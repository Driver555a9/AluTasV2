#include "tas/globalstate/MemoryAddressState.h"

#include "tas/memory/MemoryAddressFinder.h"
#include "tas/memory/MemoryUtility.h"

#include "core/utility/Assert.h"

#include <sstream>
#include <thread>

namespace AsphaltTas
{
/////////////////////////////////////////
// RacerStateAddresses
/////////////////////////////////////////
    bool RacerStateAddresses::UpdateAddresses() noexcept
    {
        try 
        { 
            ManuallySetAddresses(MemoryAddressFinder::FindRacerStateBaseAddress());
            return true;
        }
        catch (MemoryUtility::MemoryManipFailedException& e) 
        {
            ManuallySetAddresses(0);
            return false;
        }
    }

    void RacerStateAddresses::ManuallySetAddresses(uintptr_t base) noexcept 
    {
        s_base_address.store(base, std::memory_order::release);
    }

    bool RacerStateAddresses::AddressesAreValid() noexcept 
    {
        return GetBaseAddress() != INVALID_ADDRESS;
    }

    std::string RacerStateAddresses::ToString() noexcept
    {
        if (GetBaseAddress() == INVALID_ADDRESS)
        {
            return std::string("Invalid : Base\nInvalid : Trans Matrix\nInvalid : Velocity");
        }
        std::ostringstream ss;
        ss << "0x" << std::hex << GetBaseAddress() << " : Base\n"
        << "0x" << std::hex << GetTransMatrixAddress() << " : Trans Matrix\n"
        << "0x" << std::hex << GetVelocityVec3Address() << " : Velocity";
        return ss.str();
    }

    uintptr_t RacerStateAddresses::GetBaseAddress() noexcept
    {
        return s_base_address.load(std::memory_order::acquire);
    }
    
    uintptr_t RacerStateAddresses::GetTransMatrixAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_TRANS_MATRIX4x4;
    }

    uintptr_t RacerStateAddresses::GetVelocityVec3Address() noexcept
    {
        return GetBaseAddress() + OFFSET_VELOCITY_VEC3;
    }

/////////////////////////////////////////
// ActionCameraStateAddresses
/////////////////////////////////////////
    bool CameraStateAddresses::UpdateAddresses() noexcept
    {
        try 
        { 
            ManuallySetAddresses(MemoryAddressFinder::FindCameraStateAddresses()); 
            return true;
        }
        catch (MemoryUtility::MemoryManipFailedException& e) 
        {
            ManuallySetAddresses(0);
            return false;
        }
    }

    void CameraStateAddresses::ManuallySetAddresses(uintptr_t base) noexcept 
    {
        s_base_address.store(base, std::memory_order::release);
    }

    bool CameraStateAddresses::AddressesAreValid() noexcept 
    {
        return GetBaseAddress() != INVALID_ADDRESS;
    }

    std::string CameraStateAddresses::ToString() noexcept
    {
        if (GetBaseAddress() == INVALID_ADDRESS)
        {
            return std::string("Invalid : Base\nInvalid : Position\nInvalid : Rotation\nInvalid : Fov Radians\nInvalid : Aspect Ratio");
        }
        std::ostringstream ss;
        ss << "0x" << std::hex << GetBaseAddress() << " : Base\n" 
        << "0x" << std::hex << GetPositionVec3Address() << " : Position\n" 
        << "0x" << std::hex << GetRotationQuatAddress() << " : Rotation\n"
        << "0x" << std::hex << GetFovRadiansAddress()   << " : Fov Radians\n"
        << "0x" << std::hex << GetAspectRatioAddress()  << " : Aspect Ratio\n";

        return ss.str();
    }

    uintptr_t CameraStateAddresses::GetBaseAddress() noexcept
    {
        return s_base_address.load(std::memory_order::acquire);
    }

    uintptr_t CameraStateAddresses::GetPositionVec3Address() noexcept
    {
        return GetBaseAddress() + OFFSET_POSITON_VEC3;
    }

    uintptr_t CameraStateAddresses::GetRotationQuatAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_ROTATION_QUAT;
    }

    uintptr_t CameraStateAddresses::GetNearPlaneAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_NEAR_PLANE;
    }

    uintptr_t CameraStateAddresses::GetFovRadiansAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_FOV_RADIANS;
    }

    uintptr_t CameraStateAddresses::GetAspectRatioAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_ASPECT_RATIO;
    }

}