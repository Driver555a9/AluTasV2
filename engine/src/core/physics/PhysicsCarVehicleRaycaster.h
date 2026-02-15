#pragma once

#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Vehicle/btRaycastVehicle.h"

namespace CoreEngine
{
    class PhysicsCarVehicleRaycaster : public btVehicleRaycaster 
    {
    public:
        explicit PhysicsCarVehicleRaycaster(btDynamicsWorld* world, btCollisionObject* me) noexcept : m_own_car_object(me), m_world_ptr(world) {}

        virtual void* castRay(const btVector3& from, const btVector3& to, btVehicleRaycasterResult& result) override 
        {
            btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
            
            rayCallback.m_collisionFilterMask  = ~0;
            rayCallback.m_collisionFilterGroup = ~0;
            
            struct FilterCallback : public btCollisionWorld::ClosestRayResultCallback 
            {
                btCollisionObject* m_ignore;

                FilterCallback(const btVector3& from, const btVector3& to, btCollisionObject* ignore) 
                : btCollisionWorld::ClosestRayResultCallback(from, to), m_ignore(ignore) {}
                
                virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override 
                {
                    if (rayResult.m_collisionObject == m_ignore)
                        return 1.0; // Ignore this hit
                    return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
                }
            };

            FilterCallback filterCallback(from, to, m_own_car_object);
            m_world_ptr->rayTest(from, to, filterCallback);

            if (filterCallback.hasHit()) 
            {
                result.m_distFraction     = filterCallback.m_closestHitFraction;
                result.m_hitPointInWorld  = filterCallback.m_hitPointWorld;
                result.m_hitNormalInWorld = filterCallback.m_hitNormalWorld;
                result.m_hitNormalInWorld.normalize();
                return (void*)filterCallback.m_collisionObject;
            }
            return 0;
        }
    private:
        btCollisionObject* m_own_car_object;
        btDynamicsWorld* m_world_ptr;
};
}