#pragma once

#include "core/utility/Units.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

namespace AsphaltTas
{
    class RacerState 
    {
    public:
        constexpr RacerState() noexcept = default;
        constexpr RacerState(glm::mat4 trans, glm::vec3 velocity) noexcept : m_transform(trans), m_velocity(velocity) {}

        [[nodiscard]] constexpr glm::vec3 GetVelocity() const noexcept 
        {
            return m_velocity;
        }

        constexpr void SetVelocity(glm::vec3 velocity) noexcept 
        {
            m_velocity = velocity;
        }

        [[nodiscard]] constexpr glm::mat4 GetTransformMatrix() const noexcept 
        {
            return m_transform; 
        }

        constexpr void SetTransformMatrix(glm::mat4 trans) noexcept
        {
            m_transform = trans;
        }

        [[nodiscard]] constexpr glm::vec3 GetExtractedPosition() const noexcept 
        {
            // Game: [3][0] = X, [3][1] = Z , [3][2] = Y
            // Converted to: X, Y, Z convention
            return glm::vec3( m_transform[3][0], m_transform[3][2], m_transform[3][1] );
        }

        [[nodiscard]] constexpr glm::quat GetExtractedRotation() const noexcept 
        {
            auto toRenderer = [](glm::vec3 v) { return glm::vec3(v.x, v.z, v.y); };

            glm::vec3 gameRight   = glm::normalize(toRenderer(glm::vec3(m_transform[0])));
            glm::vec3 gameUp      = glm::normalize(toRenderer(glm::vec3(m_transform[2])));
            glm::vec3 gameForward = glm::normalize(toRenderer(glm::vec3(m_transform[1])));

            glm::mat3 basis;
            basis[0] = gameRight;
            basis[1] = gameUp;
            basis[2] = gameForward; 

            return glm::normalize(glm::quat_cast(basis))  * glm::angleAxis(glm::radians(-90.0f), glm::vec3(0,1,0));
        }

        [[nodiscard]] constexpr glm::vec3 GetExtractedEulerAngles() const noexcept 
        {
            return glm::degrees(glm::eulerAngles(GetExtractedRotation()));
        }

    private:
        glm::mat4 m_transform{};
        glm::vec3 m_velocity{};
    };
}