#pragma once

#include <cassert>
#include <iostream>

#include "core/utility/ColorCodes.h"

#define ENGINE_ASSERT(expr) assert(expr)

#define ENGINE_DEBUG_PRINT(expr) std::cerr << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::YELLOW) \
<< "File: " << __FILE_NAME__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::GREEN) \
<< " Line: " << __LINE__ << " : " << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RED) \
<< expr << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RESET) << std::endl

#define ENGINE_DEBUG_PRINT_NO_TEXT() std::cerr << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::YELLOW) \
<< "File: " << __FILE_NAME__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::GREEN) \
<< " Line: " << __LINE__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RESET) << std::endl

