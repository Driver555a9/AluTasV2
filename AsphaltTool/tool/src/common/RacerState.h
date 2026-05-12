#pragma once

#include "Communication.h"
#include "core/utility/Units.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "globalstate/AsphaltDllManager.h"

#include <string>

namespace AsphaltTas
{
    class RacerState 
    {
    public:
    
    explicit RacerState() noexcept = default;
    explicit RacerState(const ComDllIn::WriteRacerState& state) noexcept;
    explicit RacerState(const ComDllOut::RecordedRacerState& state) noexcept;

    [[nodiscard]] ComDllIn::WriteRacerState ToWriteRacerState() noexcept;
    [[nodiscard]] ComDllOut::RecordedRacerState ToRecordedRacerState() noexcept;

    [[nodiscard]] glm::vec3 GetVelocityGameloft_XZY() const noexcept;
    void SetVelocityGameloft_XZY(const glm::vec3& velocity) noexcept;

    [[nodiscard]] glm::vec3 GetVelocityOpenGL_XYZ() const noexcept;
    void SetVelocityOpenGL_XYZ(const glm::vec3& velocity) noexcept;

    [[nodiscard]] glm::vec3 GetPositionOpenGL_XYZ() const noexcept;
    void SetPositionOpenGL_XYZ(const glm::vec3& position) noexcept;

    [[nodiscard]] glm::quat GetRotationOpenGL_XYZ() const noexcept;
    void SetRotationOpenGL_XYZ(const glm::quat& rotation) noexcept;

    [[nodiscard]] glm::mat4 GetTransformMatrixGameloft_XZY() const noexcept;
    void SetTransformMatrixGameloft_XZY(const glm::mat4& trans) noexcept;

    [[nodiscard]] std::string ToString() const noexcept;

    [[nodiscard]] bool Equals(const RacerState& other) const noexcept;

    private:
        glm::mat4 m_transform{};
        glm::vec3 m_velocity{};
    };
}