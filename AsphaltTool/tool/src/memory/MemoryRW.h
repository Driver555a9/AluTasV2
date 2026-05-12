#pragma once

/*

#include "core/utility/Units.h"

#include "common/RacerState.h"
#include "common/CameraState.h"
#include "common/InputState.h"

namespace AsphaltTas
{
    namespace MemoryRW
    {    
        // Will not override if flag is set
        enum IGNORE_FLAG_CAMERA : unsigned int
        {
            NONE         = 0,
            PositionVec3 = 1 << 0,
            RotationQuat = 1 << 1,
            FovFloat     = 1 << 2,
            AspectRatio  = 1 << 3,
        };

    //////////////////////////////////////////////////////////
    // These functions may throw MemoryUtility::MemoryManipFailedException
    //////////////////////////////////////////////////////////

        //Automatically swaps into XYZ convention
        //Read
        [[deprecated("Prefer DLL")]] [[nodiscard]] RacerState ReadRacerState(); 
        [[deprecated("Prefer DLL")]] [[nodiscard]] CameraState ReadCameraState();
        [[deprecated("Prefer DLL")]] [[nodiscard]] CoreEngine::Units::MicroSecond ReadCurrentRaceTime();
        [[deprecated("Prefer DLL")]] [[nodiscard]] float ReadCurrentRaceProgress();
        [[deprecated("Prefer DLL")]] [[nodiscard]] InputState ReadInputState();

        //Automatically swaps back to XZY convention
        //Write
        [[deprecated("Prefer DLL")]] void WriteRacerState(const RacerState& state);
        [[deprecated("Prefer DLL")]] void WriteCameraState(const CameraState& state, IGNORE_FLAG_CAMERA ignore_flags);

        //Prevents game from updating the camera itself
        //Erase code
        [[deprecated("Prefer DLL")]] void DestroyCameraUpdateCode();
        [[deprecated("Prefer DLL")]] void RestoreCameraUpdateCode();
        [[deprecated("Prefer DLL")]] [[nodiscard]] bool CameraCodeIsDestroyed() noexcept;

        [[deprecated("Prefer DLL")]] void MakeGameDeterministic(float target_fps);
        [[deprecated("Prefer DLL")]] void RestoreGameNonDeterministic();
        [[deprecated("Prefer DLL")]] [[nodiscard]] bool GameIsDeterministic() noexcept;

        //Calling will prevent later usage of RestoreCameraUpdateCode()
        [[deprecated("Prefer DLL")]] void InvalidateCache() noexcept;
    };
}

*/