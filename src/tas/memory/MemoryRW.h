#pragma once

#include <stdexcept>

#include "tas/common/RacerState.h"
#include "tas/common/CameraState.h"

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
        [[nodiscard]] RacerState ReadRacerState(); 
        [[nodiscard]] CameraState ReadCameraState();
        
        //Automatically swaps back to XZY convention
        void WriteRacerState(const RacerState& state);
        void WriteCameraState(const CameraState& state, IGNORE_FLAG_CAMERA ignore_flags);

        //Prevents game from updating the camera itself
        void DestroyCameraUpdateCode();
        void RestoreCameraUpdateCode();
        [[nodiscard]] bool CameraCodeIsDestroyed() noexcept;

        //Calling will prevent later usage of RestoreCameraUpdateCode()
        void InvalidateCache() noexcept;
    };
}