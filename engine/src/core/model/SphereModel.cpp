#include "core/model/SphereModel.h"

#include "core/model/ModelPresets.h"

namespace CoreEngine
{
    SphereModel::SphereModel(float radius, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept : m_radius (radius), m_color(color)
    {
        const SimpleSphere_radius1& instance = SimpleSphere_radius1::GetInstance();
        std::vector<Vertex> verts = instance.m_sphere_verts;
        for (Vertex& v : verts)
        {
            v.m_position *= radius;
        }
        const MaterialPBR material { .m_base_color_factor = color };
        m_mesh_vector.emplace_back(std::move(verts), std::vector<GLuint>(instance.m_sphere_indices), std::make_shared<MaterialPBR>(material));
        m_position = position;
        m_rotation = rotation;

        m_aabb_half_extents = glm::vec3(radius);
    }

    std::unique_ptr<Basic_Model> SphereModel::Copy() const noexcept
    {
        return std::make_unique<SphereModel>(*this);
    }

    Basic_Model::ModelType SphereModel::GetModelType() const noexcept
    {
        return Basic_Model::ModelType::PRIMITIVE_SPHERE;
    }

    float SphereModel::GetUnscaledRadius() const noexcept
    {
        return m_radius;
    }

    glm::vec3 SphereModel::GetColor() const noexcept
    {
        return m_color;
    }
}