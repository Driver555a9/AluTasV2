#include "core/utility/PhysicsUtility.h"

namespace CoreEngine::PhysicsUtility
{
    btQuaternion GlmQuatToBt(const glm::quat& q)
    {
        return btQuaternion(q.x, q.y, q.z, q.w);
    }
    
    glm::quat BtQuatToGlm(const btQuaternion& q)
    {
        return glm::quat(q.w(), q.x(), q.y(), q.z());
    }

    glm::vec3 BtVector3ToGlm(const btVector3& _vec)
    {
        return glm::vec3(_vec.getX(), _vec.getY(), _vec.getZ());
    }

    btVector3 GlmVec3ToBt(const glm::vec3& _vec)
    {
        return btVector3(_vec.x, _vec.y, _vec.z);
    }

    
}