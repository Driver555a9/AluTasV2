#pragma once

//Own includes
#include "Scene3D_SceneObject.h"

namespace CoreEngine
{
    class Scene3D_ObjectBuilder final
    {
    private:
        friend class Scene3D;
        btDiscreteDynamicsWorld*          m_physics_world  = nullptr;

        glm::vec3                         m_position       = {};
        glm::quat                         m_rotation       = {};
        std::string                       m_obj_name       = "Unnamed";

        std::unique_ptr<Basic_Model>      m_render_model   = nullptr;

        PhysicsObjectType                 m_physics_type   = PhysicsObjectType::NONE;
        PhysicsObjectConfig               m_physics_object_config;

        explicit Scene3D_ObjectBuilder(btDiscreteDynamicsWorld* physics_world) noexcept;

        [[nodiscard]] std::unique_ptr<Scene3D_SceneObject> Finalize() noexcept;

    public:
    //////////////////////////////////////////////// 
    //--------- Required to call at least some
    //////////////////////////////////////////////// 
        void RenderModel_SetFromPath(const std::string& path, const glm::vec3& scaleFactor) noexcept;
        void RenderModel_SetExisting(std::unique_ptr<Basic_Model> model) noexcept;

        void CollisionShape_SetFromExisting(const btCollisionShape* shape) noexcept;
        void CollisionShape_SetBoxFromRenderModelExtents() noexcept;
        void CollisionShape_SetStaticTriangleMeshFromModel() noexcept;

        void RenderAndCollision_SetFromPoints(std::vector<Vertex>&& points, const glm::vec3& color) noexcept;
        void RenderAndCollision_SetBox(const glm::vec3& half_extents, const glm::vec3& color) noexcept;
        void RenderAndCollision_SetSphere(const float radius, const glm::vec3& color) noexcept;

        void SetPhysicsObjectType(const PhysicsObjectType type) noexcept;

    //////////////////////////////////////////////// 
    //---------  Optional to call
    //////////////////////////////////////////////// 
        void SetMass(const Units::Kilogram mass) noexcept;
        void SetPosition(const glm::vec3& pos)   noexcept;
        void SetRotation(const glm::quat& rot)   noexcept;
        void SetName(const std::string& name)    noexcept;

    //////////////////////////////////////////////// 
    //---------  Construction happens in place - no move or copy
    //////////////////////////////////////////////// 
        Scene3D_ObjectBuilder()                                        = delete;
        Scene3D_ObjectBuilder(const Scene3D_ObjectBuilder&)            = delete;
        Scene3D_ObjectBuilder& operator=(const Scene3D_ObjectBuilder&) = delete;
        Scene3D_ObjectBuilder(Scene3D_ObjectBuilder&&)                 = delete;
        Scene3D_ObjectBuilder& operator=(Scene3D_ObjectBuilder&&)      = delete;
    };
}