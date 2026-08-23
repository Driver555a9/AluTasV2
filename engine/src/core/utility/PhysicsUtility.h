#pragma once

//Bullet
#include <btBulletDynamicsCommon.h>
//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace CoreEngine
{
    namespace PhysicsUtility
    {
        [[nodiscard]] inline btVector3 ToBt(const glm::vec3& v) noexcept
        {
            return { v.x, v.y, v.z };
        }

        [[nodiscard]] inline btQuaternion ToBt(const glm::quat& q) noexcept
        {
            return { q.x, q.y, q.z, q.w };
        }

        [[nodiscard]] inline glm::vec3 ToGlm(const btVector3& v) noexcept
        {
            return { v.x(), v.y(), v.z() };
        }

        [[nodiscard]] inline glm::quat ToGlm(const btQuaternion& q) noexcept
        {
            return { q.w(), q.x(), q.y(), q.z() };
        }
    };

    namespace PUtil = PhysicsUtility;
}