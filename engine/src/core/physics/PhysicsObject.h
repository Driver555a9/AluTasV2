#pragma once

//Bullet
#include "btBulletDynamicsCommon.h"

//std
#include <memory>

//own
#include "core/physics/OwnedBvhTriangleMeshShape.h"
#include "core/physics/OwnedCompoundShape.h"

#include "core/utility/Units.h"
#include "core/utility/InputState.h"

namespace CoreEngine
{
    enum class PhysicsObjectType : int
    {
        NONE = 0,
        Ordinary,
        Car
    };

    enum class PhysicsShapeType { NONE = 0, BOX, SPHERE, TRIANGLE_MESH_SHAPE, OWNED_COMPOUND_SHAPE };

    struct PhysicsObjectConfig
    {
        btScalar m_mass               = 0.0f;
        btScalar m_friction           = 0.5f;
        btScalar m_restitution        = 0.0f;
        btScalar m_linearDamping      = 0.0f;
        btScalar m_angularDamping     = 0.0f;
        int      m_collision_group    = 1;
        int      m_collision_mask     = -1; 

        btTransform m_start_transform;
        btVector3   m_local_scaling = btVector3(1, 1, 1);

        PhysicsShapeType m_shape_type = PhysicsShapeType::NONE;

        //Box
        btVector3 m_box_half_extents = btVector3(0,0,0);

        //Sphere
        btScalar  m_sphere_radius    = 1.0f;

        //Custom mesh
        std::unique_ptr<OwnedBvhTriangleMeshShape>  m_triangle_mesh_shape = nullptr;
    };

    class PhysicsObject
    {
    public:
        explicit PhysicsObject(btDiscreteDynamicsWorld* world, PhysicsObjectConfig&& config) noexcept;
        explicit PhysicsObject(PhysicsObject&& other) noexcept;

        virtual ~PhysicsObject() noexcept;

        //////////////////////////////////////////////// 
        //--------- Public methods
        //////////////////////////////////////////////// 
        [[nodiscard]] virtual std::unique_ptr<PhysicsObject> Copy() const noexcept;
        [[nodiscard]] virtual PhysicsObjectType GetType() const noexcept;
        virtual void OnInput(const InputState& input_state, const Units::MicroSecond dt) noexcept;
        [[nodiscard]] Units::Meters_per_sec GetVelocityMS() const noexcept;
        
        [[nodiscard]] PhysicsShapeType GetShapeType() const noexcept;

        [[nodiscard]] btRigidBody*       GetBody()   noexcept;
        [[nodiscard]] btCollisionShape*  GetShape()  noexcept;
        [[nodiscard]] btMotionState*     GetMotion() noexcept;

        void SetKinematic(bool kinematic) noexcept;
        [[nodiscard]] bool IsKinematic() const noexcept;

        void ResetMotion() noexcept;

        void SetWeight(const Units::Kilogram mass);

    protected:
        std::unique_ptr<btCollisionShape> m_collision_shape;
        std::unique_ptr<btMotionState>    m_motion_state;
        std::unique_ptr<btRigidBody>      m_body;
        btDiscreteDynamicsWorld*          m_world_ptr = nullptr;

        PhysicsShapeType                  m_shape_type;

        //////////////////////////////////////////////// 
        //--------- Deleted Copy & move assign
        //////////////////////////////////////////////// 
        PhysicsObject& operator=(PhysicsObject&&)      = delete;
        PhysicsObject& operator=(const PhysicsObject&) = delete;
        PhysicsObject(const PhysicsObject& other)      = delete;
    };
}