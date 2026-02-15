#include "core/physics/PhysicsCar.h"

#include "core/physics/OwnedCompoundShape.h"
#include "core/physics/PhysicsCarVehicleRaycaster.h"

//std
#include "core/utility/Assert.h"

namespace CoreEngine
{
    //Absolute magic number hell of death. Horrendously awful
    PhysicsCar::PhysicsCar(btDiscreteDynamicsWorld* world, PhysicsObjectConfig&& config) noexcept 
    : PhysicsObject(world, std::move(config))
    {
        ENGINE_ASSERT(m_collision_shape->getShapeType() == BOX_SHAPE_PROXYTYPE && "m_collision_shape must be a btBoxShape initially.");

        //////////////////////////////////////////////// 
        //--------- Create compound shape
        //////////////////////////////////////////////// 
        m_collision_shape->setLocalScaling(btVector3(1.0f, 0.4f, 1.0f)); // Lower height, since we move the box up later - it should not hit objects above car
        const btVector3 box_half_extents = static_cast<btBoxShape*>(m_collision_shape.get())->getHalfExtentsWithMargin();

        const btVector3 center_of_mass_offset {0, box_half_extents.getY(), 0.0f}; //Offset of everything, so the center of gravity moves down relatively

        const float wheel_radius {0.4f}; // Radius of the Raycast Vehicles wheels

        const float skid_half_width   {0.12f}; // The half width of the skid wheels
        const float axle_y_pos        { center_of_mass_offset.getY() }; // The position of the axle

        const float suspension_rest_length = 0.2f; 

        btTransform chassis_local_offset; // Position of the Box. We move it up by it's half extents + the center of mass offset we apply to everything
        chassis_local_offset.setIdentity();
        chassis_local_offset.setOrigin(btVector3(0, box_half_extents.getY() + center_of_mass_offset.getY(), 0));

        std::unique_ptr<OwnedCompoundShape> compound_ptr = std::make_unique<OwnedCompoundShape>(std::move(m_collision_shape), chassis_local_offset);
        // Bigger claimed offset -> lower visual model
        compound_ptr->SetCenterOfMassOffset(center_of_mass_offset + btVector3(0, suspension_rest_length, 0)); // Can be used to insure Render model lines up with physics

        const btVector3 wheel_positions[4] = {
            btVector3( box_half_extents.x() * 0.75f, axle_y_pos,  box_half_extents.z() * 0.6f), // Front Left
            btVector3(-box_half_extents.x() * 0.75f, axle_y_pos,  box_half_extents.z() * 0.6f), // Front Right
            btVector3( box_half_extents.x() * 0.75f, axle_y_pos, -box_half_extents.z() * 0.6f), // Rear Left
            btVector3(-box_half_extents.x() * 0.75f, axle_y_pos, -box_half_extents.z() * 0.6f)  // Rear Right
        };

        for(int i = 0; i < 4; ++i)
        {
            auto skid_shape = std::make_unique<btCylinderShapeX>(btVector3(skid_half_width, wheel_radius * 0.7, wheel_radius * 0.7));
            
            btTransform skid_trans;
            skid_trans.setIdentity();
            skid_trans.setOrigin(wheel_positions[i]);
            
            compound_ptr->AddShape(std::move(skid_shape), skid_trans);
        }

        //////////////////////////////////////////////// 
        //--------- Finalize collision shape
        ////////////////////////////////////////////////
        m_collision_shape = std::move(compound_ptr);
        m_shape_type = PhysicsShapeType::OWNED_COMPOUND_SHAPE;

        m_world_ptr->removeRigidBody(m_body.get());
        m_body->setCollisionShape(m_collision_shape.get());

        //////////////////////////////////////////////// 
        //---------  Calculate inertia
        //////////////////////////////////////////////// 
        btVector3 local_inertia(0, 0, 0);
        if (config.m_mass > 0.0f) 
        {
            m_collision_shape->calculateLocalInertia(config.m_mass, local_inertia);
        }
        m_body->setMassProps(config.m_mass, local_inertia);
        m_world_ptr->addRigidBody(m_body.get(), config.m_collision_group, config.m_collision_mask);

        m_body->setFriction(0.0f); // Raycast vehicle has its own friction

        //////////////////////////////////////////////// 
        //---------  Setup Raycast vehicle
        //////////////////////////////////////////////// 
        m_raycaster = std::make_unique<PhysicsCarVehicleRaycaster>(m_world_ptr, m_body.get());
        m_body->setActivationState(DISABLE_DEACTIVATION);

        m_vehicle = std::make_unique<btRaycastVehicle>(m_tuning, m_body.get(), m_raycaster.get());
        m_vehicle->setCoordinateSystem(0, 1, 2); // X=Right, Y=Up, Z=Forward
        m_world_ptr->addVehicle(m_vehicle.get());

        const btVector3 wheel_direction(0, -1, 0);
        const btVector3 wheel_axle(-1, 0, 0);

        for (int i = 0; i < 4; ++i)
        {
            const bool is_front_wheel = (i < 2);

            btVector3 ray_connect_pos = wheel_positions[i];
            ray_connect_pos.setY(axle_y_pos);

            m_vehicle->addWheel(
                ray_connect_pos,
                wheel_direction,
                wheel_axle,
                suspension_rest_length,
                wheel_radius,
                m_tuning,
                is_front_wheel
            );

            btWheelInfo& wheel = m_vehicle->getWheelInfo(i);
            wheel.m_suspensionStiffness      = 60.0f;
            wheel.m_wheelsDampingCompression = 10.0f;
            wheel.m_wheelsDampingRelaxation  = 15.0f;
            wheel.m_frictionSlip             = 40.0f;
            wheel.m_rollInfluence            = 0.0f;
            wheel.m_maxSuspensionForce       = 1'000'000.0f;
            wheel.m_maxSuspensionTravelCm    = 40.0f;
        }
    }

    PhysicsCar::PhysicsCar(PhysicsCar&& _other) noexcept 
    : PhysicsObject(std::move(_other)), m_raycaster(std::move(_other.m_raycaster)), m_vehicle(std::move(_other.m_vehicle)), m_tuning(std::move(_other.m_tuning)) {}

    PhysicsCar::~PhysicsCar() noexcept
    {
        if (m_vehicle && m_world_ptr) 
        { 
            m_world_ptr->removeVehicle(m_vehicle.get());
        }

        // ~PhysicsObject() will remove RigidBody
    }

    std::unique_ptr<PhysicsObject> PhysicsCar::Copy() const noexcept
    {
        PhysicsObjectConfig config;
        config.m_mass               = m_body->getMass();
        config.m_friction           = m_body->getFriction();
        config.m_restitution        = m_body->getRestitution();
        config.m_linearDamping      = m_body->getLinearDamping();
        config.m_angularDamping     = m_body->getAngularDamping();

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

        config.m_shape_type         = PhysicsShapeType::BOX;

        ENGINE_ASSERT (m_collision_shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && m_shape_type == PhysicsShapeType::OWNED_COMPOUND_SHAPE
                       && "At PhysicsCar::Copy(): m_collision_shape must be compound shape." );

        OwnedCompoundShape* compound_shape = static_cast<OwnedCompoundShape*>(m_collision_shape.get());

        ENGINE_ASSERT (compound_shape->GetFirstBaseShape()->getShapeType() == BOX_SHAPE_PROXYTYPE && "At PhysicsCar::Copy(): Base Shape must be Box.");

        const btBoxShape* box_shape = static_cast<const btBoxShape*>(compound_shape->GetFirstBaseShape()); //We know this has to be box given PhysicsCar() constructor
        config.m_box_half_extents = box_shape->getHalfExtentsWithoutMargin() / box_shape->getLocalScaling(); 

        return std::make_unique<PhysicsCar>(m_world_ptr, std::move(config));
    }

    PhysicsObjectType PhysicsCar::GetType() const noexcept 
    {
        return PhysicsObjectType::Car;
    }

    void PhysicsCar::OnInput(const InputState& input_state, const Units::MicroSecond dt) noexcept
    {
        //TODO: Improve this to not rely on OnInput() being called
        ApplyDownforce();

        const auto& key_pressed = input_state.m_key_is_pressed;
        //////////////////////////////////////////////// 
        //---------  Acceleration / Braking
        //////////////////////////////////////////////// 
        Units::Newtons engine_force (0.0f);
        Units::Newtons brake_force  (0.0f);

        if (key_pressed[GLFW_KEY_W] || key_pressed[GLFW_KEY_UP])
        {
            engine_force += CalculateEngineForce(); 
        }
        if (key_pressed[GLFW_KEY_S] || key_pressed[GLFW_KEY_DOWN])
        {
            if (m_vehicle->getCurrentSpeedKmHour() >= 1.0f)
            {
                brake_force = CalculateBrakeForce();
            }
            else 
                engine_force -= CalculateEngineForce() * 0.2f;
        }

        Accelerate(Units::Newtons(engine_force));
        Brake(Units::Newtons(brake_force));

        //////////////////////////////////////////////// 
        //--------- Steering
        //////////////////////////////////////////////// 
        const Units::Degrees max_steer_angle = CalculateMaxSteerAngle();
        constexpr const Units::Degrees steer_speed (200.0f);
        
        if (key_pressed[GLFW_KEY_A] || key_pressed[GLFW_KEY_LEFT])
        {
            m_steering_angle += Units::Convert<Units::Second>(dt).Get() * steer_speed;
        }
        else if (key_pressed[GLFW_KEY_D] || key_pressed[GLFW_KEY_RIGHT])
        {
            m_steering_angle -= Units::Convert<Units::Second>(dt).Get() * steer_speed;
        }
        else 
        {
            const float return_amount = Units::Convert<Units::Second>(dt).Get() * steer_speed.Get();
            if (m_steering_angle.Get() > return_amount) 
                m_steering_angle -= Units::Degrees(return_amount);
            else if (m_steering_angle.Get() < -return_amount) 
                m_steering_angle += Units::Degrees(return_amount);
            else 
                m_steering_angle = Units::Degrees(0.0f);
        }

        m_steering_angle = std::clamp(m_steering_angle, -1.0f * max_steer_angle, max_steer_angle);
        SteerDegrees(m_steering_angle);
    }

    bool PhysicsCar::IsWheelOnGround(int wheel_index) const noexcept
    {
        ENGINE_ASSERT(wheel_index >= 0 && wheel_index < m_vehicle->getNumWheels() && ("At IsWheelOnGround(): Wheel with doesn't exist"));

        const btWheelInfo& wheel = m_vehicle->getWheelInfo(wheel_index);
        return wheel.m_raycastInfo.m_groundObject != nullptr;
    }

    btRaycastVehicle* PhysicsCar::GetRaycastVehicle() noexcept
    {
        return m_vehicle.get();
    }

    //////////////////////////////////////////////// 
    //--------- Internal helper functions
    ////////////////////////////////////////////////
    void PhysicsCar::Accelerate(const Units::Newtons force) noexcept
    {
        for (int i = 0; i < m_vehicle->getNumWheels(); ++i)
        {
            m_vehicle->applyEngineForce(force.Get(), i);
        }
    }

    void PhysicsCar::Brake(const Units::Newtons brake_force) noexcept
    {
        const float speed_kmh    = m_vehicle->getCurrentSpeedKmHour();
        const float speed_factor = std::clamp(speed_kmh / 100.0f, 0.5f, 1.0f);
        const float final_brake  = brake_force.Get() * speed_factor;

        for (int i = 0; i < m_vehicle->getNumWheels(); ++i)
        {
            if (m_vehicle->getWheelInfo(i).m_bIsFrontWheel)
                m_vehicle->setBrake(final_brake * 0.7f * 0.5f, i);
            else 
                m_vehicle->setBrake(final_brake * 0.3f * 0.5f, i);
        }

        if (speed_kmh > 50.0f && brake_force > Units::Newtons(100.0f) && IsWheelOnGround(2) && IsWheelOnGround(3))
        {
            const float downforce_magnitude = 40.0f;
            const btVector3 down_vector = -1.0f * m_body->getWorldTransform().getBasis().getColumn(1);
            const btVector3 force = down_vector * downforce_magnitude;
            btVector3 min, max;
            m_collision_shape->getAabb(m_body->getWorldTransform(), min, max);
            const btVector3 size = (max - min);
            const btVector3 rear_offset(0.0f, 0.0f, -size.z() * 0.4f);    
            m_body->applyForce(force,  rear_offset);
        } 
    }

    void PhysicsCar::SteerDegrees(const Units::Degrees angle) noexcept
    {
        btScalar steer_rad = btRadians(angle.Get());

        //////////////////////////////////////////////// 
        //--------- Turn raycast wheels
        //////////////////////////////////////////////// 
        for (int i = 0; i < m_vehicle->getNumWheels(); ++i)
        {
            if (m_vehicle->getWheelInfo(i).m_bIsFrontWheel)
            {
                m_vehicle->setSteeringValue(steer_rad, i);
            }
        }
        
        //////////////////////////////////////////////// 
        //--------- Turn skid front wheels
        //////////////////////////////////////////////// 
        OwnedCompoundShape* compound = static_cast<OwnedCompoundShape*>(m_collision_shape.get());

        btQuaternion steerRot(btVector3(0, 1, 0), steer_rad);

        ///TODO: Improve this, we assume 1 & 2 are the front wheels (0 = box) - generalize if possible
        if (compound->getNumChildShapes() < 3) return;
        
        for (int i : {1, 2}) 
        {
            btTransform localTrans = compound->getChildTransform(i); 
            localTrans.setRotation(steerRot);
            
            compound->updateChildTransform(i, localTrans);
        }
        
        m_world_ptr->updateSingleAabb(m_body.get());
    }

    void PhysicsCar::ApplyDownforce() noexcept
    {
        if (IsAirborne()) return;

        const Units::Meters_per_sec velocity (GetVehicleVelocityMS());

        constexpr const float downforce_coefficient = 70.0f;
        const float force_magnitude = downforce_coefficient * velocity.Get() * velocity.Get();

        const btVector3 down_vector = -1.0f * m_body->getWorldTransform().getBasis().getColumn(1);
        const btVector3 force = down_vector * force_magnitude;

        m_body->applyCentralForce(force);
    }

    Units::Newtons PhysicsCar::CalculateBrakeForce() const noexcept
    {
        const float base_brake = 500.0f;
        return Units::Newtons(base_brake * (m_body->getMass() / 2000.0f));
    }

    Units::Newtons PhysicsCar::CalculateEngineForce() const noexcept
    {
        const float v = GetVelocityMS().Get();

        float a_scale = 1.0f / (1.0f + pow(v / 25.0f, 1.3f));  

        const float base_force = 16'000.0f;
        return Units::Newtons(base_force * (m_body->getMass() / 2000.0f) * a_scale);
    }

    Units::Degrees PhysicsCar::CalculateMaxSteerAngle() const noexcept
    {
        const Units::Meters_per_sec velocity (GetVehicleVelocityMS());

        const Units::Degrees calculated_steer ( 45 / (velocity.Get() + 1.0f));

        return std::clamp(calculated_steer, Units::Degrees(1.0f), Units::Degrees (20.0f));
    }

    Units::Meters_per_sec PhysicsCar::GetVehicleVelocityMS() const noexcept
    {
        return Units::Meters_per_sec(m_body->getLinearVelocity().length() / 3.6f);
    }

    bool PhysicsCar::IsAirborne() const noexcept
    {
        for (int i : {0, 1, 2, 3})
        {
            if (IsWheelOnGround(i))
                return false;
        }
        return true;
    }
}