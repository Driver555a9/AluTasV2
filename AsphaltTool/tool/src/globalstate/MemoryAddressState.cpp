/*

#include "globalstate/MemoryAddressState.h"

#include "memory/MemoryAddressFinder.h"
#include "memory/MemoryUtility.h"

#include <sstream>

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
            ManuallySetAddresses(INVALID_ADDRESS);
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
        ss << "0x" << std::hex << std::uppercase << GetBaseAddress() << " : Base\n"
        << "0x" << std::hex << std::uppercase << GetTransMatrixAddress() << " : Trans Matrix\n"
        << "0x" << std::hex << std::uppercase << GetVelocityVec3Address() << " : Velocity";
        return ss.str();
    }

    uintptr_t RacerStateAddresses::GetBaseAddress() noexcept
    {
        return s_base_address.load(std::memory_order::acquire);
    }
    
    uintptr_t RacerStateAddresses::GetTransMatrixAddress() noexcept
    {
        return GetBaseAddress() + OFFSET_TRANS_MATRIX;
    }

    uintptr_t RacerStateAddresses::GetVelocityVec3Address() noexcept
    {
        return GetBaseAddress() + OFFSET_VELOCITY_VEC3;
    }

/////////////////////////////////////////
// CameraStateAddresses
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
            ManuallySetAddresses(INVALID_ADDRESS);
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
        ss << "0x" << std::hex << std::uppercase << GetBaseAddress() << " : Base\n" 
        << "0x" << std::hex << std::uppercase << GetPositionVec3Address() << " : Position\n" 
        << "0x" << std::hex << std::uppercase << GetRotationQuatAddress() << " : Rotation\n"
        << "0x" << std::hex << std::uppercase << GetFovRadiansAddress()   << " : Fov Radians\n"
        << "0x" << std::hex << std::uppercase << GetAspectRatioAddress()  << " : Aspect Ratio\n";

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

/////////////////////////////////////////
// RaceProgressStateAddresses
/////////////////////////////////////////
    bool RaceProgressStateAddresses::UpdateAddresses() noexcept
    {
        try 
        {
            SetLapTimeAddress(MemoryAddressFinder::FindLapTimeAddress());
            return true;
        } 
        catch (MemoryUtility::MemoryManipFailedException& e)
        {
            SetLapTimeAddress(INVALID_ADDRESS);
            return false;
        }
    }

    void RaceProgressStateAddresses::SetLapTimeAddress(uintptr_t lap_time) noexcept
    {
        s_lap_time_address.store(lap_time, std::memory_order::release);
    }

    void RaceProgressStateAddresses::SetRaceProgressAddress(uintptr_t progress) noexcept
    {
        s_race_progress_address.store(progress, std::memory_order::release);
    }

    void RaceProgressStateAddresses::SetCheckpointAddress(uintptr_t cp) noexcept
    {
        s_checkpoint_address.store(cp, std::memory_order::release);
    }

    std::string RaceProgressStateAddresses::ToString() noexcept
    {
        std::ostringstream ss;

        const auto AddAddress = [&ss](uintptr_t (*func)(), const char* label) -> void {
            const uintptr_t addrr = func();
            addrr == INVALID_ADDRESS ? (ss << "Invalid : " << label << "\n") : (ss << "0x" << std::hex << std::uppercase << addrr << " : " << label << "\n");
        };

        AddAddress(&GetLapTimeAddress, "Lap Time");
        AddAddress(&GetRaceProgressAddress, "Progress");
        AddAddress(&GetCheckpointAddress, "Checkpoint");

        return ss.str();
    }

    uintptr_t RaceProgressStateAddresses::GetLapTimeAddress() noexcept
    {
        return s_lap_time_address.load(std::memory_order::acquire);
    }

    uintptr_t RaceProgressStateAddresses::GetRaceProgressAddress() noexcept
    {
        return s_race_progress_address.load(std::memory_order::acquire);
    }

    uintptr_t RaceProgressStateAddresses::GetCheckpointAddress() noexcept
    {
        return s_checkpoint_address.load(std::memory_order::acquire);
    }


/////////////////////////////////////////
// InputDetourDataStateAddresses
////////////////////////////    
    void InputDataStateAddresses::SetDataBaseAddress(uintptr_t data) noexcept
    {
        s_data_address.store(data, std::memory_order::release);
    }

    bool InputDataStateAddresses::AddressesAreValid() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) != INVALID_ADDRESS;
    }

    std::string InputDataStateAddresses::ToString() noexcept
    {
        std::ostringstream ss;

        const auto AddAddress = [&ss](uintptr_t base, uintptr_t offset, const char* label) -> void {
            base == INVALID_ADDRESS ? (ss << "Invalid : " << label << "\n") : (ss << "0x" << std::hex << std::uppercase << (base + offset) << " : " << label << "\n");
        };

        uintptr_t base = s_data_address.load(std::memory_order::relaxed);
        AddAddress(base, OFFSET_PACKET_ID_4BYTES, "PacketID");
        return ss.str();
    }

    uintptr_t InputDataStateAddresses::GetPacketIDAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_PACKET_ID_4BYTES;
    }
    
    uintptr_t InputDataStateAddresses::GetButtonMaskAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_BUTTON_MASK_2BYTES;
    }

    uintptr_t InputDataStateAddresses::GetLeftTriggerAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_LEFT_TRIGGER_1BYTE;
    }
    uintptr_t InputDataStateAddresses::GetRightTriggerAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_RIGHT_TRIGGER_1BYTE;
    }
    uintptr_t InputDataStateAddresses::GetLeftStickXAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_LEFT_STICK_X_2BYTES;
    }
    uintptr_t InputDataStateAddresses::GetLeftStickYAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_LEFT_STICK_Y_2BYTES;
    }

    uintptr_t InputDataStateAddresses::GetRightStickXAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_RIGHT_STICK_X_2BYTES;
    }
    uintptr_t InputDataStateAddresses::GetRightStickYAddress() noexcept
    {
        return s_data_address.load(std::memory_order::acquire) + OFFSET_RIGHT_STICK_X_2BYTES;
    }

}


*/