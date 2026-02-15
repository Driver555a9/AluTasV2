#pragma once

#include "core/model/Mesh.h"

#include "core/utility/MathUtility.h"

namespace CoreEngine
{
    class Basic_Model 
    {
    public:
        enum class ModelType 
        { 
            NONE = 0, 
            
            PATH_MODEL, 
            PRIMITIVE_BOX, 
            PRIMITIVE_SPHERE, 
            POINTS_MODEL, 

            FIRST = PATH_MODEL, 
            LAST  = POINTS_MODEL
        };

        virtual ~Basic_Model() noexcept = default;

        [[nodiscard]] virtual std::unique_ptr<Basic_Model> Copy()   const noexcept = 0; //Textures will be shared
        [[nodiscard]] virtual Basic_Model::ModelType GetModelType() const noexcept = 0;

        [[nodiscard]] glm::mat4 GetModelMatrix() const noexcept;

        [[nodiscard]] std::vector<Mesh>& GetMeshVectorReference() noexcept;
        [[nodiscard]] const std::vector<Mesh>& GetMeshVectorConstReference() const noexcept;

        [[nodiscard]] glm::vec3 GetPosition()      const noexcept;
        [[nodiscard]] glm::quat GetRotation()      const noexcept;
        [[nodiscard]] glm::vec3 GetScale()         const noexcept;

        [[nodiscard]] glm::vec3 GetLocalCenter()      const noexcept;
        [[nodiscard]] glm::vec3 GetLocalAABBHalfExtents() const noexcept;

        [[nodiscard]] float GetWorldSpaceMaximumBoundingSphereRadius() const noexcept;
        [[nodiscard]] float GetLocalSpaceMaximumBoundingSphereRadius() const noexcept;

        [[nodiscard]] MathUtility::AABB GetWorldSpaceAABB() const noexcept;
        [[nodiscard]] MathUtility::AABB GetLocalSpaceAABB() const noexcept;

        void SetPosition(const glm::vec3& _pos)             noexcept;
        void SetRotation(const glm::quat& _rot)             noexcept;
        void SetScale(const glm::vec3& _scale)              noexcept;

    protected:
        std::vector<Mesh> m_mesh_vector;
        glm::vec3 m_position {0.0f};
        glm::quat m_rotation {};
        glm::vec3 m_scale {1.0f};

        glm::vec3 m_local_center {0.0f};
        glm::vec3 m_aabb_half_extents;

        void CalculateAABBExtentsAndLocalCenter() noexcept;
        void CenterModelLocally() noexcept;

        explicit Basic_Model() noexcept = default;

        //Copy / Move policy
        Basic_Model(const Basic_Model&)            noexcept = default;
        Basic_Model(Basic_Model&&)                 noexcept = default;
        Basic_Model& operator=(const Basic_Model&) noexcept = default;
        Basic_Model& operator=(Basic_Model&&)      noexcept = default;
    };

}