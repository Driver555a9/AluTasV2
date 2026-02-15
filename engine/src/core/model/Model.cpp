#include "core/model/Model.h"

//Standard imports
#include <array>

namespace CoreEngine
{
    glm::mat4 Basic_Model::GetModelMatrix() const noexcept
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // Apply translation
        modelMatrix = glm::translate(modelMatrix, m_position);

        // Apply quaternion rotation
        modelMatrix *= glm::toMat4(m_rotation);

        // Apply scale
        modelMatrix = glm::scale(modelMatrix, m_scale);

        return modelMatrix;
    }

    void Basic_Model::CenterModelLocally() noexcept
    {
        if (m_local_center == glm::vec3(0.0f))
        {
            return;
        }

        for (Mesh& mesh : m_mesh_vector)
        {
            for (Vertex& vertex : mesh.GetVerticesReference())
            {
                vertex.m_position -= m_local_center;
            }

            //Insure up to date local center in mesh
            mesh.SetLocalCenter(mesh.GetLocalCenter() -= m_local_center);
        }

        m_local_center = glm::vec3(0.0f);
    }

    void Basic_Model::CalculateAABBExtentsAndLocalCenter() noexcept
    {
        glm::vec3 max {std::numeric_limits<float>::lowest()};
        glm::vec3 min {std::numeric_limits<float>::max()};

        for(const Mesh& mesh : m_mesh_vector)
        {
            for(const Vertex& vertex : mesh.GetVerticesConstReference())
            {
                max = glm::max(max, vertex.m_position);
                min = glm::min(min, vertex.m_position);
            }
        }

        m_aabb_half_extents = (max - min) / 2.0f;
        m_local_center      = (max + min) / 2.0f;
    }

    glm::vec3 Basic_Model::GetPosition() const noexcept
    {
        return m_position;
    }

    glm::quat Basic_Model::GetRotation() const noexcept
    {
        return m_rotation;
    }

    glm::vec3 Basic_Model::GetScale() const noexcept
    {
        return m_scale;
    }

    float Basic_Model::GetWorldSpaceMaximumBoundingSphereRadius() const noexcept
    {
        return glm::length(m_aabb_half_extents * m_scale);
    }

    float Basic_Model::GetLocalSpaceMaximumBoundingSphereRadius() const noexcept
    {
        return glm::length(m_aabb_half_extents);
    }

    MathUtility::AABB Basic_Model::GetWorldSpaceAABB() const noexcept
    {
        return MathUtility::AABB::CreateWorldSpaceAABB (GetModelMatrix(), m_aabb_half_extents, m_local_center);   
    }

    MathUtility::AABB Basic_Model::GetLocalSpaceAABB() const noexcept
    {
        return MathUtility::AABB(m_aabb_half_extents, m_local_center);
    }

    glm::vec3 Basic_Model::GetLocalCenter() const noexcept
    {
        return m_local_center;
    }

    glm::vec3 Basic_Model::GetLocalAABBHalfExtents() const noexcept
    {
        return m_aabb_half_extents;
    }

    const std::vector<Mesh>& Basic_Model::GetMeshVectorConstReference() const noexcept
    {
        return m_mesh_vector;
    }

    std::vector<Mesh>& Basic_Model::GetMeshVectorReference() noexcept
    {
        return m_mesh_vector;
    }

    void Basic_Model::SetPosition(const glm::vec3& _pos) noexcept
    {
        m_position = _pos;
    }   

    void Basic_Model::SetRotation(const glm::quat& _rot) noexcept
    {
        m_rotation = _rot;
    }

    void Basic_Model::SetScale(const glm::vec3& _scale) noexcept
    {
        m_scale = _scale;
    }
}