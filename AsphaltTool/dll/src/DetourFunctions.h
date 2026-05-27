#pragma once

#include <cstdint>

namespace AsphaltDLL
{
    namespace DetourFunctions 
    {
        enum class HookState
        {
            NotInPlace, InPlaceDisabled, InPlaceEnabled
        };

        // Internal, however OnHandleGeneralInBuffer may be called by external threads
        // Must LOCK_CURRENT_STATE_MUTEX() before call
        namespace StateManager
        {
            void OnHandleGeneralInBuffer() noexcept;
        }

        ////////////////////////////////////////////////////////////
        // Function called on each new logical frame. 
        // R8 = elapsed delta time in microseconds
        // Handles both game side logic (nitro, drift etc.) and calls Physics Update function
        // Does not run whilst in pause menu for whatever reason
        ////////////////////////////////////////////////////////////
        namespace NewPhysicsFrameFunction 
        {
            void QueueSkipSubsequentTicks(uint32_t amount) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////////////////////
        // Function called on each Bullet Physics step. 
        // [rdx] is float seconds interval time from last call
        // Function internally increments a counter with [rdx] and does as many physics ticks until that counter is < 0, subtracting PF each tick
        ////////////////////////////////////////////////////////////
        namespace NewBulletPhysicsTick
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Function writes final steer input normalized between [-1;+1]
        ///////////////////////////////////////
        namespace SteeringValue
        {
            bool SpoofCallToSteerValueFunction(float steer) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Function writes final brake value [-1 brake; +1 none]
        // First gameplay function we hook, therefore it initiates the new frame
        ///////////////////////////////////////
        namespace BrakeValue
        {
            bool SpoofCallToBrakeValueFunction(float brake) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Huge functions that does lots of shit, including setting accelerator
        // Accelerator at RCX + 0x1D0
        ///////////////////////////////////////
        namespace AcceleratorValue
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Function is called when any nitro button is pressed
        // RCX parameter = NitroState; if provided we can spoof a call to enable nitro without relying on input
        ///////////////////////////////////////
        namespace EnableNitro 
        {
            // Nitro func won't run unless called, therefore it can not manage its own state each time (New frame func will manage it)
            [[nodiscard]] uint8_t GetAndResetNitroFunctionWasCalledCounter() noexcept;
            void SpoofCallToEnableNitroFunction(std::uint32_t count) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Decrease of nitro goes through here
        // Writes encrypted 12 bytes [RCX+0x18C]
        // Function also does other shit but who cares
        ///////////////////////////////////////
        namespace DecreaseNitroBarFunc
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }
        
        ///////////////////////////////////////
        // Increase of nitro goes through here
        // Writes encrypted 12 bytes [RCX+0x18C]
        // Function also does other shit but who cares
        ///////////////////////////////////////
        namespace IncreaseNitroBarFunc
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Utility that enables simple access to nitro bar state
        // No function detour
        ///////////////////////////////////////
        namespace UpdateNitroBar 
        {
            bool WriteNitroBar(float value) noexcept;
            [[nodiscard]] float ReadNitroBar() noexcept;
        }

        ///////////////////////////////////////
        // Function updates transform of car
        // [rcx] = local racer base pointer
        ///////////////////////////////////////
        namespace RacerTransformUpdate
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Function useful for deducing local racer struct;
        // RacerTransformUpdate() may be called with multiple dynamic objects
        ///////////////////////////////////////
        namespace GetLocalRacerStruct
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Calls Set position (rax+80)
        // Calls Set rotation (rax+88)
        // Sets fov rad (movss [rcx+00000128],xmm0)
        ///////////////////////////////////////
        namespace CameraUpdate
        {
            void PatchDisableGameFovWriteInstruction() noexcept;
            void PatchEnableGameFovWriteInstruction() noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Indeterministic functions that stabilize the car while in barrel roll
        // Likely caused by garbage initialized object passed into function
        // Only called whilst car is doing a barrel roll, only called once per PF tick
        // Functions write to angular velocity
        ///////////////////////////////////////
        namespace BarrelRollStabilization
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        namespace BarrelYawStabilization
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Getter that returns 1/PF - Physics cycle interval as float secs
        ///////////////////////////////////////
        namespace GetPhysicsInterval
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Begin Race function
        // Arbitrary function called when beginning a race, to detect "is in race" state
        ///////////////////////////////////////
        namespace OnBeginRaceFunction
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // Arbitrary function called when clicking play or restart
        ///////////////////////////////////////
        namespace OnClickPlayFunction
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////
        // End Race function
        // Arbitrary function called when ending a race, to detect "is in race" state
        // Called precisely at Fin line if race isn't aborted
        ///////////////////////////////////////
        namespace OnEndRaceFunction
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////
        // Sets float value obfuscated as 3 * 4 bytes into the destination directly
        // Destination must be final game storage address
        // Mind that this may only be called on the same thread as where the value is read again
        ////////////////////////////////////////////
        namespace FloatXorObfuscationSetter
        {
            void EncryptToAddress(void* p_dest, float value) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////
        // Reads float value obfuscated as 3 * 4 bytes and returns it
        // Mind that this may only be called on the same thread as where the value was written
        ////////////////////////////////////////////
        namespace FloatXorObfuscationGetter
        {
            [[nodiscard]] float DecryptFromAddress(void* original_addr) noexcept;
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////
        // Function returns 0 or 1 to render or hide all of the GUI
        ////////////////////////////////////////////
        namespace RenderGUIToggle
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////
        // This function runs on the games main thread
        // Dispatches logic for new frame (including Physics worker thread)
        // On this same thread runs camera and cp logic
        ////////////////////////////////////////////
        namespace MainLoopNewFrameDispatcher
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ///////////////////////////////////////////
        // Animation progress function, useful for skipping animations
        ///////////////////////////////////////////
        namespace AnimationProgressFunction
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        ////////////////////////////////////////////
        // XInputGetState() gives controller state
        ////////////////////////////////////////////
        namespace XInput_GetState
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }

        /////////////////////////////////////////
        // UcrtBaseRand
        // E.g. used for wreck physics / wreck camera
        /////////////////////////////////////////
        namespace UcrtBaseRand
        {
            bool SetupHook() noexcept;
            bool RemoveHook() noexcept;
            bool EnableHook() noexcept;
            bool DisableHook() noexcept;
            [[nodiscard]] HookState GetHookState() noexcept;
        }
    }
}
