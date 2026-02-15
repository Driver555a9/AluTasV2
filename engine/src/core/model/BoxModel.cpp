#include "core/model/BoxModel.h"

#include "ModelPresets.h"

namespace CoreEngine
{
    BoxModel::BoxModel(glm::vec3 half_extents,  glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept : m_color(color)
    {
        std::vector<Vertex> verts = SimpleCube_1x1x1::CUBE_VERTICES;
        for (Vertex& v : verts)
        {
            v.m_position *= (half_extents * 2.0f);
        }

        const MaterialPBR material { .m_base_color_factor = color };
        m_mesh_vector.emplace_back(std::move(verts), std::vector<GLuint>(SimpleCube_1x1x1::CUBE_INDICES), std::make_shared<MaterialPBR>(material));
        
        m_position = position;
        m_rotation = rotation;
        m_aabb_half_extents = half_extents;
    }

    std::unique_ptr<Basic_Model> BoxModel::Copy() const noexcept
    {
        return std::make_unique<BoxModel>(*this);
    }

    Basic_Model::ModelType BoxModel::GetModelType() const noexcept
    {
        return Basic_Model::ModelType::PRIMITIVE_BOX;
    }

    glm::vec3 BoxModel::GetColor() const noexcept
    {
        return m_color;
    }

}