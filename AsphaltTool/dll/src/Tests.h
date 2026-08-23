#pragma once
#include <string>

namespace BulletTypes
{
    class CollisionObject;
}

namespace AsphaltDLL
{
    namespace Tests
    {
        void LoadCustomTrack() noexcept;

        void PrintCollisionObjectTest(BulletTypes::CollisionObject* obj) noexcept;

        void ChangeMaterialsTest() noexcept;
        
        void DebugDumpPhysicsWorldObjects(const std::string& path) noexcept;
        
        void MovePhysicsObjectsTest() noexcept;

        void RaycastTest() noexcept;
    }
}
