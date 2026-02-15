#pragma once

#include <cstdint>
#include <string>
#include <atomic>

namespace AsphaltTas
{
    constexpr inline uintptr_t INVALID_ADDRESS = 0x0;
    
    class RacerStateAddresses
    {
    public:
        RacerStateAddresses() noexcept = delete;
        
        static bool UpdateAddresses() noexcept;
        static void ManuallySetAddresses(uintptr_t base) noexcept;

        static void LaunchAddressUpdateServiceThread() noexcept;
        static void StopAddressUpdateServiceThread() noexcept;
        [[nodiscard]] static bool GetAddressUpdateServiceThreadIsRunning() noexcept;

        [[nodiscard]] static bool AddressesAreValid() noexcept;

        [[nodiscard]] static std::string ToString() noexcept;

        [[nodiscard]] static uintptr_t GetBaseAddress() noexcept;
        [[nodiscard]] static uintptr_t GetTransMatrixAddress() noexcept;
        [[nodiscard]] static uintptr_t GetVelocityVec3Address() noexcept;

        [[nodiscard]] static std::pair<uintptr_t, uintptr_t> GetMinMaxDataAddressSpace() noexcept;

        ////////////////////////////////////////
        // Offsets relative to base address
        ////////////////////////////////////////
        constexpr const static inline uintptr_t OFFSET_TRANS_MATRIX4x4  = 0x20;
        constexpr const static inline uintptr_t OFFSET_VELOCITY_VEC3    = 0x160;

    private:
        static inline std::atomic<uintptr_t> s_base_address = INVALID_ADDRESS;
        static inline std::atomic<bool> s_address_update_service_thread_is_running = false;
    };

    class CameraStateAddresses
    {
    public:
        CameraStateAddresses() noexcept = delete;

        static bool UpdateAddresses() noexcept;
        static void ManuallySetAddresses(uintptr_t base) noexcept;

        static void LaunchAddressUpdateServiceThread() noexcept;
        static void StopAddressUpdateServiceThread() noexcept;
        [[nodiscard]] static bool GetAddressUpdateServiceThreadIsRunning() noexcept;

        [[nodiscard]] static bool AddressesAreValid() noexcept;

        [[nodiscard]] static std::string ToString() noexcept;

        [[nodiscard]] static uintptr_t GetBaseAddress() noexcept;

        [[nodiscard]] static uintptr_t GetPositionVec3Address() noexcept;
        [[nodiscard]] static uintptr_t GetRotationQuatAddress() noexcept;
        [[nodiscard]] static uintptr_t GetFovRadiansAddress()   noexcept;
        [[nodiscard]] static uintptr_t GetAspectRatioAddress()  noexcept;

        [[nodiscard]] static std::pair<uintptr_t, uintptr_t> GetMinMaxDataAddressSpace() noexcept;

        ////////////////////////////////////////
        // Offsets relative to base Address
        ////////////////////////////////////////
        constexpr static const inline uintptr_t OFFSET_POSITON_VEC3  = 0x0;
        constexpr static const inline uintptr_t OFFSET_ROTATION_QUAT = 0xC;
        constexpr static const inline uintptr_t OFFSET_FOV_RADIANS   = 0xF0;
        constexpr static const inline uintptr_t OFFSET_ASPECT_RATIO  = 0xF8;

    private:
        static inline std::atomic<uintptr_t> s_base_address = INVALID_ADDRESS;
        static inline std::atomic<bool> s_address_update_service_thread_is_running = false;
    };
}