#include "BulletTypes.h"
#include <string>

namespace AsphaltDLL
{
    namespace Tests
    {
        void PrintCollisionObjectTest(BulletTypes::CollisionObject* obj) noexcept;

        void ChangeMaterialsTest() noexcept;
        
        void DebugDumpPhysicsWorldObjects(const std::string& path) noexcept;
        
        void MovePhysicsObjectsTest() noexcept;

        void RaycastTest() noexcept;
    }
}
