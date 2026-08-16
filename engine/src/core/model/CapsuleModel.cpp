#include "core/model/CapsuleModel.h"

#include "core/model/Model.h"
#include "core/model/ModelPresets.h"

namespace CoreEngine
{
    CapsuleModel::CapsuleModel(float radius, float cylinder_height, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept 
        : m_radius(radius), m_cylinder_height(cylinder_height), m_color(color)
    {
        const SimpleCapsule_r1_h2& instance = SimpleCapsule_r1_h2::GetInstance();
        std::vector<Vertex> verts = instance.m_verts;
        
        for (Vertex& v : verts)
        {
            v.m_position.x *= radius;
            v.m_position.z *= radius;

            if (v.m_position.y > 0.0f)
            {
                v.m_position.y = (v.m_position.y - 1.0f) * radius + (cylinder_height * 0.5f);
            } else if (v.m_position.y < 0.0f) 
            {
                v.m_position.y = (v.m_position.y + 1.0f) * radius - (cylinder_height * 0.5f);
            }
        }

        const MaterialPBR material { .m_base_color_factor = color };
        m_mesh_vector.emplace_back(std::move(verts), std::vector<GLuint>(instance.m_indices), std::make_shared<MaterialPBR>(material));
        
        m_position = position;
        m_rotation = rotation;
        m_aabb_half_extents = glm::vec3(radius, (cylinder_height * 0.5f) + radius, radius);
        CalculateAABBExtentsAndLocalCenter();
    }

    std::unique_ptr<Basic_Model> CapsuleModel::Copy() const noexcept 
    {
        return std::make_unique<CapsuleModel>(*this);
    }

    Basic_Model::ModelType CapsuleModel::GetModelType() const noexcept 
    {
        return Basic_Model::ModelType::CAPSULE_MODEL;
    }

    float CapsuleModel::GetUnscaledRadius() const noexcept
    {
        return m_radius;
    }

    float CapsuleModel::GetCylinderHeight() const noexcept
    {
        return m_cylinder_height;
    }

    glm::vec3 CapsuleModel::GetColor() const noexcept
    {
        return m_color;
    }

}