#include "common/RacerState.h"

#include "core/utility/CommonUtility.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/orthonormalize.hpp"

namespace AsphaltTas
{
    RacerState::RacerState(const ComDllIn::WriteRacerState& state) noexcept
    {
        std::memcpy(glm::value_ptr(m_transform), state.m_racer_transform_mat4x4.data(), sizeof(decltype(m_transform)));
        std::memcpy(glm::value_ptr(m_velocity), state.m_racer_velocity_vec3.data(), sizeof(decltype(m_velocity)));
    }
    
    RacerState::RacerState(const ComDllOut::RecordedRacerState& state) noexcept
    {
        std::memcpy(glm::value_ptr(m_transform), state.m_racer_transform_mat4x4.data(), sizeof(decltype(m_transform)));
        std::memcpy(glm::value_ptr(m_velocity), state.m_racer_velocity_vec3.data(), sizeof(decltype(m_velocity)));
    }

    ComDllIn::WriteRacerState RacerState::ToWriteRacerState() noexcept
    {
        ComDllIn::WriteRacerState out;

        std::memcpy(out.m_racer_transform_mat4x4.data(), glm::value_ptr(m_transform), sizeof(decltype(m_transform)));
        std::memcpy(out.m_racer_velocity_vec3.data(), glm::value_ptr(m_velocity), sizeof(decltype(m_velocity)));

        return out;
    }

    ComDllOut::RecordedRacerState RacerState::ToRecordedRacerState() noexcept
    {
        ComDllOut::RecordedRacerState out;

        std::memcpy(out.m_racer_transform_mat4x4.data(), glm::value_ptr(m_transform), sizeof(decltype(m_transform)));
        std::memcpy(out.m_racer_velocity_vec3.data(), glm::value_ptr(m_velocity), sizeof(decltype(m_velocity)));

        return out;
    }

    glm::vec3 RacerState::GetVelocityGameloft_XZY() const noexcept 
    {
        return m_velocity;
    }

    void RacerState::SetVelocityGameloft_XZY(const glm::vec3& velocity) noexcept 
    {
        m_velocity = velocity;
    }

    glm::vec3 RacerState::GetVelocityOpenGL_XYZ() const noexcept 
    {
        glm::vec3 xzy = m_velocity;
        xzy.z *= -1.0f;
        std::swap(xzy.y, xzy.z);
        return xzy;
    }

    void RacerState::SetVelocityOpenGL_XYZ(const glm::vec3& velocity) noexcept 
    {
        m_velocity = velocity;
        m_velocity.z *= -1.0f;
        std::swap(m_velocity.y, m_velocity.z);
    }

    glm::vec3 RacerState::GetPositionOpenGL_XYZ() const noexcept 
    {
        ///////////////////////////////////////////
        // Convert to X, Y, Z convention & invert z for glm -z convention
        //////////////////////////////////////////
        return glm::vec3( m_transform[3][0], m_transform[3][2], -1.0f * m_transform[3][1] );
    }

    void RacerState::SetPositionOpenGL_XYZ(const glm::vec3& position) noexcept
    {
        m_transform[3][0] = position.x;
        m_transform[3][2] = position.y;
        m_transform[3][1] = position.z * -1.0f;
    }

    glm::quat RacerState::GetRotationOpenGL_XYZ() const noexcept 
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

    void RacerState::SetRotationOpenGL_XYZ(const glm::quat& rotation) noexcept
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

    glm::mat4 RacerState::GetTransformMatrixGameloft_XZY() const noexcept
    {
        return m_transform;
    }

    void RacerState::SetTransformMatrixGameloft_XZY(const glm::mat4& trans) noexcept
    {
        m_transform = trans;
    }

    std::string RacerState::ToString() const noexcept
    {
        return "Pos: " + CoreEngine::CommonUtility::GlmVec3ToString(GetPositionOpenGL_XYZ()) + "\nRot: " + CoreEngine::CommonUtility::GlmQuatToString(GetRotationOpenGL_XYZ());
    }

    bool RacerState::Equals(const RacerState& other) const noexcept
    {
        return m_transform == other.m_transform && m_velocity == other.m_velocity;
    }
}