#pragma once

//std
#include <array>
#include <optional>

//glfw
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

//glm
#include "glm/glm.hpp"

namespace CoreEngine
{
    struct InputState
    {
        std::array<bool, GLFW_KEY_LAST + 1>            m_key_is_pressed {};
        std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1>   m_mouse_is_pressed {};
        std::optional<glm::ivec2>                      m_previous_mouse_pos {};
        glm::ivec2                                     m_mouse_move_delta {};
        float                                          m_mouse_wheel_scroll_delta {};
    };
}