#pragma once

//std
#include <string>

//glfw
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

//glm
#include "glm/glm.hpp"

namespace CoreEngine
{
    namespace CommonUtility
    {
        [[nodiscard]] std::string ReadFileToString(const char* filepath);

        [[nodiscard]] std::pair<double, double> GetMousePosition(GLFWwindow* window) noexcept;

        [[nodiscard]] std::pair<int, int> GetFramebufferSize(GLFWwindow* window) noexcept;

        [[nodiscard]] std::string GlmVec3ToString(const glm::vec3& vec) noexcept;

        [[nodiscard]] std::string GlmQuatToString(const glm::quat& q) noexcept;
    }

}