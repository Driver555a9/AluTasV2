#include "core/model/Mesh.h"

namespace CoreEngine
{
    Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<GLuint>&& indices, std::shared_ptr<MaterialPBR> material) noexcept 
    : m_vertices(std::move(vertices)), m_indices(std::move(indices)), m_material(std::move(material)) 
    {
        CalculateAABBExtentsAndLocalCenter();
    }

    std::vector<Vertex>& Mesh::GetVerticesReference() noexcept 
    { 
        return m_vertices; 
    }

    const std::vector<Vertex>& Mesh::GetVerticesConstReference() const noexcept 
    { 
        return m_vertices; 
    }

    std::vector<GLuint>& Mesh::GetIndicesReference() noexcept 
    {
        return m_indices;  
    }

    const std::vector<GLuint>& Mesh::GetIndicesConstReference() const noexcept 
    { 
        return m_indices;  
    }

    std::shared_ptr<MaterialPBR> Mesh::GetMaterialSharedPtr() noexcept 
    { 
        return m_material;  
    }

    const std::shared_ptr<MaterialPBR> Mesh::GetMaterialConstSharedPtr() const noexcept 
    { 
        return m_material;  
    }

    void Mesh::SetVertices(std::vector<Vertex>&& vertices) noexcept 
    { 
        m_vertices = std::move(vertices); 
        CalculateAABBExtentsAndLocalCenter(); 
    }

    void Mesh::SetIndices(std::vector<GLuint>&& indices) noexcept 
    { 
        m_indices  = std::move(indices); 
    }

    void Mesh::SetMaterial(std::shared_ptr<MaterialPBR> material) noexcept 
    { 
        m_material = material; 
    }

    void Mesh::CalculateAABBExtentsAndLocalCenter() noexcept
    {
        glm::vec3 max {std::numeric_limits<float>::lowest()};
        glm::vec3 min {std::numeric_limits<float>::max()};

        for(const Vertex& vertex : m_vertices)
        {
            max = glm::max(max, vertex.m_position);
            min = glm::min(min, vertex.m_position);
        }

        m_aabb_half_extents = (max - min) / 2.0f;
        m_local_center      = (max + min) / 2.0f;
    }

    glm::vec3 Mesh::GetLocalCenter() const noexcept
    {
        return m_local_center;
    }

    glm::vec3 Mesh::GetLocalAABBHalfExtents() const noexcept 
    { 
        return m_aabb_half_extents; 
    }

    void Mesh::SetLocalCenter(const glm::vec3& center) noexcept
    {
        m_local_center = center;
    }
}