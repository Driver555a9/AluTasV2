#include "common/RacerState.h"

#include "BulletTypes.h"
#include "core/utility/CommonUtility.h"
#include <bit>

namespace AsphaltTas
{
    RacerState::RacerState(const ComDllIn::WriteRacerState& state) noexcept
    {
        m_transform = std::bit_cast<glm::mat4>(state.m_racer_transform_mat4x4);
        m_velocity  = std::bit_cast<glm::vec3>(state.m_racer_velocity_vec3);
    }
    
    RacerState::RacerState(const ComDllOut::RecordedRacerState& state) noexcept
    {
        m_transform = std::bit_cast<glm::mat4>(state.m_racer_transform_mat4x4);
        m_velocity  = std::bit_cast<glm::vec3>(state.m_racer_velocity_vec3);
        m_race_progress_percentage = state.m_race_progress_percentage;
        m_rpm = state.m_rpm;
        m_checkpoint = state.m_checkpoint;
        m_gear = state.m_gear;
    }

    ComDllIn::WriteRacerState RacerState::ToWriteRacerState() noexcept
    {
        ComDllIn::WriteRacerState out;

        out.m_racer_transform_mat4x4 = std::bit_cast<BulletTypes::UnalignedTransform>(m_transform);
        out.m_racer_velocity_vec3    = std::bit_cast<BulletTypes::UnalignedVector3>(m_velocity);

        return out;
    }

    ComDllOut::RecordedRacerState RacerState::ToRecordedRacerState() noexcept
    {
        ComDllOut::RecordedRacerState out;

        out.m_racer_transform_mat4x4   = std::bit_cast<BulletTypes::UnalignedTransform>(m_transform);
        out.m_racer_velocity_vec3      = std::bit_cast<BulletTypes::UnalignedVector3>(m_velocity);
        out.m_race_progress_percentage = m_race_progress_percentage;
        out.m_rpm                = m_rpm;
        out.m_checkpoint               = m_checkpoint;
        out.m_gear                     = m_gear;
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

    float RacerState::GetRaceProgress() const noexcept
    {
        return m_race_progress_percentage;
    }
    void RacerState::SetRaceProgress(float prog) noexcept
    {
        m_race_progress_percentage = prog;
    }

    float RacerState::GetRpm() const noexcept
    {
        return m_rpm;
    }
    void RacerState::SetRpm(float rpm) noexcept
    {
        m_rpm = rpm;
    }

    std::uint32_t RacerState::GetCheckpoint() const noexcept
    {   
        return m_checkpoint;
    }
    void RacerState::SetCheckpoint(std::uint32_t cp) noexcept
    {
        m_checkpoint = cp;
    }

    std::uint32_t RacerState::GetGear() const noexcept
    {
        return m_gear;
    }
    void RacerState::SetGear(std::uint32_t gear) noexcept
    {
        m_gear = gear;
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
