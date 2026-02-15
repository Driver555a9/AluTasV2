#pragma once

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

//Standard imports
#include <memory>
#include <vector>

//Own imports
#include "core/rendering/Texture.h"

#include "core/rendering/Material.h"

namespace CoreEngine
{
    struct Vertex 
    {
        glm::vec3 m_position;
        glm::vec3 m_normal;
        glm::vec2 m_tex_uv;
    };

    static_assert(sizeof(Vertex) == 32, "Vertex must be 32 bytes");

    class Mesh 
    {
    public:
        explicit Mesh(std::vector<Vertex>&& vertices, std::vector<GLuint>&& indices, std::shared_ptr<MaterialPBR> material) noexcept;

        [[nodiscard]] std::vector<Vertex>& GetVerticesReference()                      noexcept;
        [[nodiscard]] const std::vector<Vertex>& GetVerticesConstReference()    const  noexcept;
        [[nodiscard]] std::vector<GLuint>& GetIndicesReference()                       noexcept;
        [[nodiscard]] const std::vector<GLuint>& GetIndicesConstReference()     const  noexcept;
        [[nodiscard]] std::shared_ptr<MaterialPBR> GetMaterialSharedPtr()                   noexcept;
        [[nodiscard]] const std::shared_ptr<MaterialPBR> GetMaterialConstSharedPtr() const  noexcept;

        void SetVertices(std::vector<Vertex>&& vertices)  noexcept;
        void SetIndices(std::vector<GLuint>&& indices)    noexcept;
        void SetMaterial(std::shared_ptr<MaterialPBR> material) noexcept;

        void CalculateAABBExtentsAndLocalCenter() noexcept;
        [[nodiscard]] glm::vec3 GetLocalCenter() const noexcept;
        [[nodiscard]] glm::vec3 GetLocalAABBHalfExtents() const noexcept;

        void SetLocalCenter(const glm::vec3& center) noexcept; //Should only be called if vertices are changed outside Mesh

    private:
        std::vector<Vertex> m_vertices;
        std::vector<GLuint> m_indices;
        std::shared_ptr<MaterialPBR> m_material;

        glm::vec3 m_local_center{};
        glm::vec3 m_aabb_half_extents;
    };
}