#pragma once

//Own includes
#include "core/physics/PhysicsObject.h"

class PhysicsCarVehicleRaycaster;

namespace CoreEngine
{
    class PhysicsCar : public PhysicsObject
    {
    public:
        explicit PhysicsCar(btDiscreteDynamicsWorld* _world, PhysicsObjectConfig&& config) noexcept;
        explicit PhysicsCar(PhysicsCar&& _other) noexcept;
        virtual ~PhysicsCar() noexcept override;

        //////////////////////////////////////////////// 
        //--------- Inherited methods from PhysicsObject
        //////////////////////////////////////////////// 

        // TODO: IMPROVE COPY !!! PhysicsCar creates OwnedCompoundShape using BoxShape - it however stores "box shape"
        [[nodiscard]] virtual std::unique_ptr<PhysicsObject> Copy() const noexcept override;
        [[nodiscard]] virtual PhysicsObjectType GetType()           const noexcept override;
        virtual void OnInput(const InputState& input_state, const Units::MicroSecond dt) noexcept override;

        [[nodiscard]] bool IsWheelOnGround(int wheel_index) const noexcept;

        [[nodiscard]] btRaycastVehicle* GetRaycastVehicle() noexcept;

    protected:
        std::unique_ptr<btVehicleRaycaster>   m_raycaster;
        std::unique_ptr<btRaycastVehicle>     m_vehicle;
        btRaycastVehicle::btVehicleTuning     m_tuning; 

        //////////////////////////////////////////////// 
        //--------- state variables
        ////////////////////////////////////////////////
        Units::Degrees m_steering_angle {0.0f};

        //////////////////////////////////////////////// 
        //--------- Internal helper functions
        ////////////////////////////////////////////////
        void Accelerate(const Units::Newtons force) noexcept;
        void Brake(const Units::Newtons brake_force) noexcept;
        void SteerDegrees(const Units::Degrees angle) noexcept;
        void ApplyDownforce() noexcept;

        [[nodiscard]] Units::Newtons CalculateBrakeForce() const noexcept;
        [[nodiscard]] Units::Newtons CalculateEngineForce() const noexcept;
        [[nodiscard]] Units::Degrees CalculateMaxSteerAngle() const noexcept;
        [[nodiscard]] Units::Meters_per_sec GetVehicleVelocityMS() const noexcept;
        [[nodiscard]] bool IsAirborne() const noexcept;

        //////////////////////////////////////////////// 
        //--------- Deleted Copy & move assign
        //////////////////////////////////////////////// 
        PhysicsCar(const PhysicsCar&)            = delete;
        PhysicsCar& operator=(const PhysicsCar&) = delete;
        PhysicsCar& operator=(PhysicsCar&&)      = delete;
    };
}