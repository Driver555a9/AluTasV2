#pragma once

#include <cstdint>
#include <string>
#include <atomic>

namespace AsphaltTas
{
    /*
    constexpr inline uintptr_t INVALID_ADDRESS = 0x0;
    
    class RacerStateAddresses
    {
    public:
        RacerStateAddresses() noexcept = delete;
        
        [[deprecated("Prefer MemoryAddressUpdateService thread")]]
        static bool UpdateAddresses() noexcept;
        static void ManuallySetAddresses(uintptr_t base) noexcept;

        [[nodiscard]] static bool AddressesAreValid() noexcept;

        [[nodiscard]] static std::string ToString() noexcept;

        [[nodiscard]] static uintptr_t GetBaseAddress() noexcept;
        [[nodiscard]] static uintptr_t GetTransMatrixAddress() noexcept;
        [[nodiscard]] static uintptr_t GetVelocityVec3Address() noexcept;

        [[nodiscard]] constexpr static size_t GetByteSizeBaseToLastElementInclusive() noexcept
        {
            // Last data + sizeof last data
            return OFFSET_VELOCITY_VEC3 + (sizeof(float) * 3); // 3 floats = glm::vec3
        }

        ////////////////////////////////////////
        // Offsets relative to base address
        ////////////////////////////////////////
        //In actuality, just a mat3x3 rot followed by vec3 position!
        constexpr const static inline uintptr_t OFFSET_TRANS_MATRIX  = 0x20;
        constexpr const static inline uintptr_t OFFSET_VELOCITY_VEC3 = 0x160;

    private:
        static inline std::atomic<uintptr_t> s_base_address = INVALID_ADDRESS;
    };

    class CameraStateAddresses
    {
    public:
        CameraStateAddresses() noexcept = delete;

        [[deprecated("Prefer MemoryAddressUpdateService thread")]]
        static bool UpdateAddresses() noexcept;
        static void ManuallySetAddresses(uintptr_t base) noexcept;

        [[nodiscard]] static bool AddressesAreValid() noexcept;

        [[nodiscard]] static std::string ToString() noexcept;

        [[nodiscard]] static uintptr_t GetBaseAddress() noexcept;

        [[nodiscard]] static uintptr_t GetPositionVec3Address() noexcept;
        [[nodiscard]] static uintptr_t GetRotationQuatAddress() noexcept;
        [[nodiscard]] static uintptr_t GetNearPlaneAddress()    noexcept;
        [[nodiscard]] static uintptr_t GetFovRadiansAddress()   noexcept;
        [[nodiscard]] static uintptr_t GetAspectRatioAddress()  noexcept;

        [[nodiscard]] constexpr static size_t GetByteSizeBaseToLastElementInclusive() noexcept
        {
            // Last data + sizeof last data
            return OFFSET_ASPECT_RATIO + sizeof(float);
        }

        ////////////////////////////////////////
        // Offsets relative to base Address
        ////////////////////////////////////////
        constexpr static const inline uintptr_t OFFSET_POSITON_VEC3  = 0x0;
        constexpr static const inline uintptr_t OFFSET_ROTATION_QUAT = 0xC;
        constexpr static const inline uintptr_t OFFSET_NEAR_PLANE    = 0x60;
        constexpr static const inline uintptr_t OFFSET_FOV_RADIANS   = 0xF0;
        constexpr static const inline uintptr_t OFFSET_ASPECT_RATIO  = 0xF8;

    private:
        static inline std::atomic<uintptr_t> s_base_address = INVALID_ADDRESS;
    };
    
    class RaceProgressStateAddresses
    {
    public:
        RaceProgressStateAddresses() noexcept = delete;

        [[deprecated("Prefer MemoryAddressUpdateService thread")]]
        static bool UpdateAddresses() noexcept;

        static void SetLapTimeAddress(uintptr_t lap_time) noexcept;
        static void SetRaceProgressAddress(uintptr_t progress) noexcept;
        static void SetCheckpointAddress(uintptr_t cp) noexcept; 
        
        [[nodiscard]] static uintptr_t GetLapTimeAddress() noexcept;
        [[nodiscard]] static uintptr_t GetRaceProgressAddress() noexcept;
        [[nodiscard]] static uintptr_t GetCheckpointAddress() noexcept;
        
        [[nodiscard]] static std::string ToString() noexcept;

    private:
        static inline std::atomic<uintptr_t> s_lap_time_address      = INVALID_ADDRESS;
        static inline std::atomic<uintptr_t> s_race_progress_address = INVALID_ADDRESS;
        static inline std::atomic<uintptr_t> s_checkpoint_address    = INVALID_ADDRESS;
    };

    class InputDataStateAddresses
    {
    public:
        InputDataStateAddresses() noexcept = delete;

        static void SetDataBaseAddress(uintptr_t data) noexcept;

        [[nodiscard]] static bool AddressesAreValid() noexcept;

        [[nodiscard]] static std::string ToString() noexcept;

        [[nodiscard]] static uintptr_t GetPacketIDAddress() noexcept;

        [[nodiscard]] static uintptr_t GetButtonMaskAddress() noexcept;

        [[nodiscard]] static uintptr_t GetLeftTriggerAddress() noexcept;
        [[nodiscard]] static uintptr_t GetRightTriggerAddress() noexcept;

        [[nodiscard]] static uintptr_t GetLeftStickXAddress() noexcept;
        [[nodiscard]] static uintptr_t GetLeftStickYAddress() noexcept;

        [[nodiscard]] static uintptr_t GetRightStickXAddress() noexcept;
        [[nodiscard]] static uintptr_t GetRightStickYAddress() noexcept;

    private:
        static inline std::atomic<uintptr_t> s_data_address = INVALID_ADDRESS;

        constexpr static inline uintptr_t OFFSET_PACKET_ID_4BYTES        = 0x0;
        //Buttons
        constexpr static inline uintptr_t OFFSET_BUTTON_MASK_2BYTES      = 0x4;
        //Triggers
        constexpr static inline uintptr_t OFFSET_LEFT_TRIGGER_1BYTE      = 0x6;
        constexpr static inline uintptr_t OFFSET_RIGHT_TRIGGER_1BYTE     = 0x7;
        //Left stick
        constexpr static inline uintptr_t OFFSET_LEFT_STICK_X_2BYTES     = 0x8;
        constexpr static inline uintptr_t OFFSET_LEFT_STICK_Y_2BYTES     = 0xA;
        //Right stick
        constexpr static inline uintptr_t OFFSET_RIGHT_STICK_X_2BYTES    = 0xC;
        constexpr static inline uintptr_t OFFSET_RIGHT_STICK_Y_2BYTES    = 0xE;
    };

     */
}