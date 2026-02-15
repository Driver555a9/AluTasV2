#pragma once

//Own includes
#include "core/model/Model.h"
#include "core/model/Light.h"

#include "core/physics/PhysicsWorld.h"
#include "core/physics/PhysicsObject.h"

//json
#include <nlohmann/json.hpp>

namespace CoreEngine
{
    class Scene3D_SceneObject final
    {
    public:
        std::unique_ptr<Basic_Model>    m_render_model   = nullptr;
        std::unique_ptr<PhysicsObject>  m_physics_object = nullptr;
        std::string                     m_name           {"Unnamed"};
        
        explicit Scene3D_SceneObject(std::unique_ptr<Basic_Model> model, std::unique_ptr<PhysicsObject> physics_obj, std::string&& name) noexcept;
        explicit Scene3D_SceneObject(const Scene3D_SceneObject& other) noexcept; 
        explicit Scene3D_SceneObject(const nlohmann::ordered_json& serialized_object, btDiscreteDynamicsWorld* physics_world) noexcept;

        explicit Scene3D_SceneObject(Scene3D_SceneObject&&)   noexcept = default;
        Scene3D_SceneObject& operator=(Scene3D_SceneObject&&) noexcept = default;
        
        Scene3D_SceneObject& operator=(const Scene3D_SceneObject& other) noexcept = delete;

        [[nodiscard]] glm::vec3 GetPosition() const noexcept;
        void SetPosition(const glm::vec3& pos) noexcept;

        [[nodiscard]] glm::quat GetRotation() const noexcept;
        void SetRotation(const glm::quat& rot) noexcept;

        [[nodiscard]] glm::vec3 GetScale() const noexcept;
        void SetScale(const glm::vec3& scale) noexcept;

        void SyncRenderTransformWithPhysics() noexcept;

        [[nodiscard]] float GetWorldSpaceMaxBoundingSphereRadius() const noexcept;

        [[nodiscard]] bool HasEitherRenderModelOrPhysicsObject() const noexcept;
        [[nodiscard]] std::vector<glm::vec3> CalculateAABBDebugLines() const noexcept;

        // Very limited and must be improved
        [[nodiscard]] nlohmann::ordered_json SerializeToJson() const noexcept;
    };
}