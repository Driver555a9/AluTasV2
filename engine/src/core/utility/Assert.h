#pragma once

#include <cassert>
#include <iostream>

#include "core/utility/ColorCodes.h"

#define ENGINE_ASSERT(expr) assert(expr)

#define ENGINE_ERROR_PRINT(expr) std::cout << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RED) \
<< "\n[ERROR] File: " << __FILE_NAME__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::GREEN) \
<< " Line " << __LINE__ << ": " << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RESET) << expr << std::endl

#define ENGINE_ERROR_PRINT_NO_TEXT() std::cout << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RED) \
<< "\n[ERROR] File: " << __FILE_NAME__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::GREEN) \
<< " Line " << __LINE__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RESET) << std::endl

#define ENGINE_INFO_LOG(expr) std::cout << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::YELLOW) \
<< "\n[INFO] File: " << __FILE_NAME__ << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::GREEN) \
<< " Line " << __LINE__ << ": " << ::CoreEngine::ColorCodes::GetColor(::CoreEngine::ColorCodes::RESET) << expr << std::endl

