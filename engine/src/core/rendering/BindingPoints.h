#pragma once

#include "glad/gl.h"

namespace CoreEngine
{
    enum SSBO_BINDING : GLuint 
    {
        IndirectDraw3D_TRANSFORM     = 0,
        IndirectDraw3D_MATERIAL      = 1,
        IndirectDraw3D_LIGHTS        = 2,
        IndirectDraw3D_DRAW_INDICES  = 3
    };

    enum UBO_BINDING : GLuint 
    {
        IndirectDraw3D_SSBO_SIZES = 1,
        IndirectDraw3D_CAMERA     = 2
    };
}