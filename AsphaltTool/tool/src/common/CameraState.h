#pragma once

#include "Communication.h"
#include "core/utility/CommonUtility.h"
#include "globalstate/AsphaltDllManager.h"

#include "glm/ext/quaternion_float.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace AsphaltTas
{
    class CameraState
    {
    public:
        explicit CameraState() noexcept = default;
        explicit CameraState(const ComDllIn::WriteCameraState& state) noexcept;
        explicit CameraState(const ComDllOut::RecordedCameraState& state) noexcept;

        [[nodiscard]] ComDllIn::WriteCameraState ToWriteCameraState() noexcept;
        [[nodiscard]] ComDllOut::RecordedCameraState ToRecordedCameraState() noexcept;

        [[nodiscard]] glm::vec3 GetPositionOpenGL_XYZ() const noexcept;
        void SetPositionOpenGL_XYZ(const glm::vec3& pos) noexcept;

        [[nodiscard]] glm::quat GetRotationOpenGL_WXYZ() const noexcept;
        void SetRotationOpenGL_WXYZ(const glm::quat& rot) noexcept;

        [[nodiscard]] glm::vec3 GetPositionGameloft_XZY() const noexcept;
        void SetPositionGameloft_XZY(const glm::vec3& pos) noexcept;

        [[nodiscard]] glm::quat GetRotationGameloft_XZYW() const noexcept;
        void SetRotationGameloft_XZYW(const glm::quat& rot) noexcept;

        [[nodiscard]] float GetFovRadians() const noexcept;
        void SetFovRadians(float fov) noexcept;

        [[nodiscard]] float GetAspectRatio() const noexcept;

        [[nodiscard]] std::string ToString() const noexcept;

    private:
        glm::vec3 m_position {0};
        glm::vec3 m_local_racer_offset {};
        float     m_fov_radians    = 1.0f;
        float     m_aspect_ratio   = 1920.0f / 1080;
        // Simply stored for the sake of avoiding data loss
        glm::quat m_rotation       = glm::identity<glm::quat>();
        bool      m_look_backwards = false; // Applied if using relative offset, simply 
    };
}