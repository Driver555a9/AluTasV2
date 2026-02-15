#pragma once

//Bullet
class btDiscreteDynamicsWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;

//std
#include <memory>

#include "core/utility/Units.h"

namespace CoreEngine
{
    class PhysicsWorld
    {
        protected:
            std::unique_ptr<btDiscreteDynamicsWorld>             m_dynamics_world;

            std::unique_ptr<btDefaultCollisionConfiguration>     m_collision_config;
            std::unique_ptr<btCollisionDispatcher>               m_dispatcher; 
            std::unique_ptr<btDbvtBroadphase>                    m_broadphase;
            std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
                
        public:
            explicit PhysicsWorld();
            virtual ~PhysicsWorld() = default;

            virtual void OnUpdate(const Units::MicroSecond delta_time);
            virtual void OnUpdate(const Units::Second delta_time);

            PhysicsWorld(PhysicsWorld&&)                 = default;
            PhysicsWorld& operator=(PhysicsWorld&&)      = default;
    //---------------------- 
            PhysicsWorld(const PhysicsWorld&)            = delete;
            PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    //---------------------- 
            [[nodiscard]] btDiscreteDynamicsWorld* GetDynamicsWorldPtr();
    };
}