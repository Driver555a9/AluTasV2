#pragma once

//Bullet
#include "btBulletDynamicsCommon.h"


#include <memory>

#include <vector>

namespace CoreEngine
{
    class OwnedCompoundShape : public btCompoundShape
    {
    public:
        explicit OwnedCompoundShape(std::unique_ptr<btCollisionShape> base_shape, const btTransform& local_offset) noexcept;

        void AddShape(std::unique_ptr<btCollisionShape> shape, const btTransform& local_offset) noexcept;

        [[nodiscard]] const btCollisionShape* GetFirstBaseShape() const noexcept;
        void SetCenterOfMassOffset(const btVector3& offset) noexcept;

        [[nodiscard]] btVector3 GetCenterOfMassOffset() const noexcept;

    private:
        std::vector<std::unique_ptr<btCollisionShape>> m_child_shapes_and_offset;
        btVector3 m_center_of_mass_offset {0, 0, 0};
    };
}