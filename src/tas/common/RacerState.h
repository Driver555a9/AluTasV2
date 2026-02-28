#pragma once

#include "core/utility/Units.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <string>

namespace AsphaltTas
{
    class RacerState 
    {
    public:
        constexpr RacerState() noexcept = default;
        RacerState(const glm::mat4& trans, const glm::vec3& velocity) noexcept;

        [[nodiscard]] glm::vec3 GetVelocity() const noexcept;
        void SetVelocity(const glm::vec3& velocity) noexcept;

        [[nodiscard]] glm::vec3 GetExtractedPosition() const noexcept;
        void SetPosition(const glm::vec3& position) noexcept;

        [[nodiscard]] glm::quat GetExtractedRotation() const noexcept;
        void SetRotation(const glm::quat& rotation) noexcept;

        [[nodiscard]] glm::mat4 GetGameConventionTransformMatrix() const noexcept;
        void SetGameConventionTransformMatrix(const glm::mat4& trans) noexcept;

        [[nodiscard]] std::string ToString() const noexcept;

        [[nodiscard]] bool Equals(const RacerState& other) const noexcept;

    private:
        glm::mat4 m_transform{};
        glm::vec3 m_velocity{};
    };
}