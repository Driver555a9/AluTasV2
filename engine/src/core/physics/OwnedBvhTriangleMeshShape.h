#pragma once

//Bullet
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

#include <memory>

namespace CoreEngine
{
    class OwnedBvhTriangleMeshShape : public btBvhTriangleMeshShape 
    {
    public:
        std::shared_ptr<btTriangleMesh> m_mesh_ptr;

        OwnedBvhTriangleMeshShape(std::shared_ptr<btTriangleMesh> mesh, const bool useQuantized) : btBvhTriangleMeshShape(mesh.get(), useQuantized), m_mesh_ptr(std::move(mesh)) {}
    };
}