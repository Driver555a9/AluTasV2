#include "core/physics/OwnedCompoundShape.h"

#include "btBulletDynamicsCommon.h"

namespace CoreEngine
{
    OwnedCompoundShape::OwnedCompoundShape(std::unique_ptr<btCollisionShape> base_shape, const btTransform& local_offset) noexcept 
    {
        AddShape(std::move(base_shape), local_offset);
    }

    void OwnedCompoundShape::AddShape(std::unique_ptr<btCollisionShape> shape, const btTransform& local_offset) noexcept
    {
        addChildShape(local_offset, shape.get());
        m_child_shapes_and_offset.push_back(std::move(shape));
    }

    const btCollisionShape* OwnedCompoundShape::GetFirstBaseShape() const noexcept 
    {
        return m_child_shapes_and_offset.front().get();
    }

    void OwnedCompoundShape::SetCenterOfMassOffset(const btVector3& offset) noexcept 
    {
        m_center_of_mass_offset = offset;
    }

    [[nodiscard]] btVector3 OwnedCompoundShape::GetCenterOfMassOffset() const noexcept
    {
        return m_center_of_mass_offset;
    }
}