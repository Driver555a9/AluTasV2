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
        [[nodiscard]] btQuaternion GlmQuatToBt(const glm::quat& q);
        
        [[nodiscard]] glm::quat BtQuatToGlm(const btQuaternion& q);

        [[nodiscard]] glm::vec3 BtVector3ToGlm(const btVector3& _vec);

        [[nodiscard]] btVector3 GlmVec3ToBt(const glm::vec3& _vec);

        template <typename TFrom>
        [[nodiscard]] constexpr inline auto GlmToBt(const TFrom& from)
        {
            if constexpr (std::same_as<TFrom, glm::vec3>)
            {
                return btVector3(from.x, from.y, from.z);
            }
            if constexpr (std::same_as<TFrom, glm::quat>)
            {
                return btQuaternion(from.x, from.y, from.z, from.w);
            }
            else 
            {
                static_assert(sizeof(TFrom) == 0, "Only glm::vec3 and glm::quat supported.");
            }
        }

        template <typename TFrom>
        [[nodiscard]] constexpr inline auto BtToGlm(const TFrom& from)
        {
            if constexpr (std::same_as<TFrom, btVector3>)
            {
                return glm::vec3(from.getX(), from.getY(), from.getZ());
            }
            if constexpr (std::same_as<TFrom, btQuaternion>)
            {
                return glm::quat(from.w(), from.x(), from.y(), from.z());
            }
            else 
            {
                static_assert(sizeof(TFrom) == 0, "Only glm::vec3 and glm::quat supported.");
            }
        }

    };
}