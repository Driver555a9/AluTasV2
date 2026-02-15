#include "core/physics/PhysicsWorld.h"

#include "btBulletDynamicsCommon.h"

namespace CoreEngine
{
    PhysicsWorld::PhysicsWorld()
    {
        m_collision_config = std::make_unique<btDefaultCollisionConfiguration>();
        m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collision_config.get());
        m_broadphase = std::make_unique<btDbvtBroadphase>();
        m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        
        m_dynamics_world = std::make_unique<btDiscreteDynamicsWorld>(m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collision_config.get());
        m_dynamics_world->setGravity(btVector3(0, -9.81f, 0));
    }

    void PhysicsWorld::OnUpdate(const Units::MicroSecond delta_time)
    {
       OnUpdate(Units::Convert<Units::Second>(delta_time));
    }

    void PhysicsWorld::OnUpdate(const Units::Second delta_time)
    {
        m_dynamics_world->stepSimulation(delta_time.Get(), 0);
    }

    btDiscreteDynamicsWorld* PhysicsWorld::GetDynamicsWorldPtr()
    {
        return m_dynamics_world.get();
    }
}