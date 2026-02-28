#include "tas/common/RacerState.h"

#include "core/utility/CommonUtility.h"
#include "glm/gtx/orthonormalize.hpp"

namespace AsphaltTas
{
    RacerState::RacerState(const glm::mat4& trans, const glm::vec3& velocity) noexcept : m_transform(trans), m_velocity(velocity) {}

    glm::vec3 RacerState::GetVelocity() const noexcept 
    {
        //MemoryRW already converts (X, Z, Y) to (X, Y, -Z)
        return m_velocity;
    }

    void RacerState::SetVelocity(const glm::vec3& velocity) noexcept 
    {
        //MemoryRW already converts (X, Y, -Z) to (X, Z, Y)
        m_velocity = velocity;
    }

    glm::vec3 RacerState::GetExtractedPosition() const noexcept 
    {
        ///////////////////////////////////////////
        // Convert to X, Y, Z convention & invert z for glm -z convention
        //////////////////////////////////////////
        return glm::vec3( m_transform[3][0], m_transform[3][2], -1.0f * m_transform[3][1] );
    }

    void RacerState::SetPosition(const glm::vec3& position) noexcept
    {
        m_transform[3][0] = position.x;
        m_transform[3][2] = position.y;
        m_transform[3][1] = position.z * -1.0f;
    }

    glm::quat RacerState::GetExtractedRotation() const noexcept 
    {
        ///////////////////////////////////////////
        // Convert to X, Z, Y convention & invert z for game +z convention
        //////////////////////////////////////////
        auto GameToGlm = [](glm::vec3 v) -> glm::vec3
        { 
            return glm::vec3( v.x, v.z, -v.y );
        };

        glm::vec3 right   = GameToGlm(glm::vec3(m_transform[0][0], m_transform[1][0], m_transform[2][0]));
        glm::vec3 forward = GameToGlm(glm::vec3(m_transform[0][1], m_transform[1][1], m_transform[2][1]));
        glm::vec3 up      = GameToGlm(glm::vec3(m_transform[0][2], m_transform[1][2], m_transform[2][2]));

        forward = glm::normalize(forward);
        right   = glm::normalize(glm::cross(up, forward));
        up      = glm::cross(forward, right);

        glm::mat3 basis;
        basis[0] = right;
        basis[1] = up;
        basis[2] = forward;

        return glm::normalize(glm::quat_cast(basis));
    }

    void RacerState::SetRotation(const glm::quat& rotation) noexcept
    {
        auto GlmToGame = [](glm::vec3 v) -> glm::vec3
        {
            return glm::vec3(v.x, -v.z, v.y);
        };

        glm::mat3 basis = glm::mat3_cast(rotation);

        glm::vec3 right   = GlmToGame(basis[0]);
        glm::vec3 up      = GlmToGame(basis[1]);
        glm::vec3 forward = GlmToGame(basis[2]);

        forward = glm::normalize(forward);
        right   = glm::normalize(glm::cross(up, forward));
        up      = glm::cross(forward, right);

        m_transform[0][0] = right.x;   m_transform[1][0] = right.y;   m_transform[2][0] = right.z;
        m_transform[0][1] = forward.x; m_transform[1][1] = forward.y; m_transform[2][1] = forward.z;
        m_transform[0][2] = up.x;      m_transform[1][2] = up.y;      m_transform[2][2] = up.z;
    }

    glm::mat4 RacerState::GetGameConventionTransformMatrix() const noexcept
    {
        return m_transform;
    }

    void RacerState::SetGameConventionTransformMatrix(const glm::mat4& trans) noexcept
    {
        m_transform = trans;
    }

    std::string RacerState::ToString() const noexcept
    {
        return "Pos: " + CoreEngine::CommonUtility::GlmVec3ToString(GetExtractedPosition()) + "\nRot: " + CoreEngine::CommonUtility::GlmQuatToString(GetExtractedRotation());
    }

    bool RacerState::Equals(const RacerState& other) const noexcept
    {
        return m_transform == other.m_transform && m_velocity == other.m_velocity;
    }
}