#include "core/physics/PhysicsObject.h"

#include <core/utility/Assert.h>

namespace CoreEngine
{
    PhysicsObject::PhysicsObject(btDiscreteDynamicsWorld* world, PhysicsObjectConfig&& config) noexcept : m_world_ptr(world), m_shape_type(config.m_shape_type)
    {
        m_shape_type = config.m_shape_type;
        if (m_shape_type == PhysicsShapeType::BOX)
        {
            m_collision_shape = std::make_unique<btBoxShape>(config.m_box_half_extents);
        }
        else if (m_shape_type == PhysicsShapeType::SPHERE)
        {
            m_collision_shape = std::make_unique<btSphereShape>(config.m_sphere_radius);
        }
        else if (m_shape_type == PhysicsShapeType::TRIANGLE_MESH_SHAPE)
        {
            ENGINE_ASSERT (config.m_triangle_mesh_shape && "At PhysicsObject::PhysicsObject(): m_triangle_mesh_data must not be null");
            config.m_mass = 0.0f;
            m_collision_shape = std::move(config.m_triangle_mesh_shape);
        }
        else 
        {
            ENGINE_ASSERT (false && "At PhysicsObject::PhysicsObject(): Invalid shape type provided.");
        }

        m_collision_shape->setLocalScaling(config.m_local_scaling);

        btVector3 localInertia(0,0,0);
        if (config.m_mass != 0.0f)
        {
            m_collision_shape->calculateLocalInertia(config.m_mass, localInertia);
        }

        m_motion_state = std::make_unique<btDefaultMotionState>(config.m_start_transform);

        btRigidBody::btRigidBodyConstructionInfo rbInfo(config.m_mass, m_motion_state.get(), m_collision_shape.get(), localInertia);

        m_body = std::make_unique<btRigidBody>(rbInfo);

        m_body->setFriction(config.m_friction);
        m_body->setRestitution(config.m_restitution);
        m_body->setDamping(config.m_linearDamping, config.m_angularDamping);

        m_world_ptr->addRigidBody(m_body.get(), config.m_collision_group, config.m_collision_mask);
    }

    PhysicsObject::PhysicsObject(PhysicsObject&& other) noexcept
    : m_collision_shape(std::move(other.m_collision_shape)),
      m_motion_state(std::move(other.m_motion_state)),
      m_body(std::move(other.m_body)),
      m_world_ptr(other.m_world_ptr),
      m_shape_type(other.m_shape_type)
    {
        other.m_world_ptr = nullptr;
    }

    PhysicsObject::~PhysicsObject()
    {
        if (m_body && m_world_ptr)
        {
            m_world_ptr->removeRigidBody(m_body.get());
        }
    }

    std::unique_ptr<PhysicsObject> PhysicsObject::Copy() const noexcept
    {
        PhysicsObjectConfig config;
        config.m_mass               = m_body->getMass();
        config.m_friction           = m_body->getFriction();
        config.m_restitution        = m_body->getRestitution();
        config.m_linearDamping      = m_body->getLinearDamping();
        config.m_angularDamping     = m_body->getAngularDamping();
        config.m_local_scaling      = m_collision_shape->getLocalScaling();

        if (auto* proxy = m_body->getBroadphaseProxy())
        {
            config.m_collision_group = proxy->m_collisionFilterGroup;
            config.m_collision_mask  = proxy->m_collisionFilterMask;
        }
        else
        {
            config.m_collision_group = 1;
            config.m_collision_mask  = -1;
        }

        config.m_start_transform    = m_body->getWorldTransform();

        config.m_shape_type         = m_shape_type;

        if (m_shape_type == PhysicsShapeType::BOX)
        {
            const btBoxShape* box   = static_cast<btBoxShape*>(m_collision_shape.get());
            config.m_box_half_extents = box->getHalfExtentsWithoutMargin() / box->getLocalScaling(); // Divide by scale because getHalfExtentsWithoutMargin() includes it
        }
        else if (m_shape_type == PhysicsShapeType::SPHERE)
        {
            const btSphereShape* sphere = static_cast<btSphereShape*>(m_collision_shape.get());
            config.m_sphere_radius      = sphere->getRadius() / sphere->getLocalScaling().x(); // Divide by scale because getRadius() includes it !!!
        }
        else if (m_shape_type == PhysicsShapeType::TRIANGLE_MESH_SHAPE)
        {
            const OwnedBvhTriangleMeshShape* shape = static_cast<OwnedBvhTriangleMeshShape*>(m_collision_shape.get());
            config.m_triangle_mesh_shape = std::make_unique<OwnedBvhTriangleMeshShape>(shape->m_mesh_ptr, true);
        }
        else 
        {
            ENGINE_ASSERT(false && "At PhysicsObject::Copy() must have a valid shape");
        }

        return std::make_unique<PhysicsObject>(m_world_ptr, std::move(config));
    }

    PhysicsObjectType PhysicsObject::GetType() const noexcept
    {
        return PhysicsObjectType::Ordinary;
    }

    Units::Meters_per_sec PhysicsObject::GetVelocityMS() const noexcept
    {
        return Units::Meters_per_sec(m_body->getLinearVelocity().length());
    }

    void PhysicsObject::OnInput(const InputState& input_state, const Units::MicroSecond dt) noexcept
    {
        //nothing
    }

    PhysicsShapeType PhysicsObject::GetShapeType() const noexcept
    {
        return m_shape_type;
    }

    btRigidBody* PhysicsObject::GetBody() noexcept
    { 
        return m_body.get();    
    }

    btCollisionShape* PhysicsObject::GetShape() noexcept
    { 
        return m_collision_shape.get(); 
    }

    btMotionState* PhysicsObject::GetMotion() noexcept
    { 
        return m_motion_state.get(); 
    }

    void PhysicsObject::SetKinematic(bool kinematic) noexcept
    {   
        if (kinematic)
        {
            m_body->setCollisionFlags(m_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            m_body->setActivationState(DISABLE_DEACTIVATION);
        }
        else 
        {
            m_body->setCollisionFlags(m_body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            m_body->setActivationState(DISABLE_DEACTIVATION);
        }
    }

    bool PhysicsObject::IsKinematic() const noexcept
    {
        return m_body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    
    void PhysicsObject::ResetMotion() noexcept
    {
        m_body->clearForces();
        m_body->clearGravity();
        m_body->setLinearVelocity(btVector3(0, 0, 0));
        m_body->setAngularVelocity(btVector3(0, 0, 0));
        m_body->setInterpolationLinearVelocity(btVector3(0, 0, 0));
        m_body->setInterpolationAngularVelocity(btVector3(0, 0, 0));
    }

    void PhysicsObject::SetWeight(const Units::Kilogram mass)
    {
        if (m_shape_type == PhysicsShapeType::TRIANGLE_MESH_SHAPE) //Must not change weight for triangle mesh shape
        {
            return;
        }

        m_world_ptr->removeRigidBody(m_body.get());

        int flags = m_body->getCollisionFlags();
        if (mass > Units::Kilogram(0.0f))
            flags &= ~btCollisionObject::CF_STATIC_OBJECT;
        else
            flags |= btCollisionObject::CF_STATIC_OBJECT;
        
        m_body->setCollisionFlags(flags);

        btVector3 inertia(0,0,0);
        if (mass > Units::Kilogram(0.0f))
            m_collision_shape->calculateLocalInertia(mass.Get(), inertia);

        m_body->setMassProps(mass.Get(), inertia);
        m_body->updateInertiaTensor();

        m_world_ptr->addRigidBody(m_body.get());

        m_body->forceActivationState(ACTIVE_TAG);
        m_body->activate(true);
    }
}