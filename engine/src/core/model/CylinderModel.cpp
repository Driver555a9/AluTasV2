#include "core/model/CylinderModel.h"

#include "core/model/Model.h"
#include "core/model/ModelPresets.h"

namespace CoreEngine
{
    CylinderModel::CylinderModel(float radius, float height, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept 
        : m_radius(radius), m_height(height), m_color(color)
    {
        const SimpleCylinder_r1_h2& instance = SimpleCylinder_r1_h2::GetInstance();
        std::vector<Vertex> verts = instance.m_verts;
        
        for (Vertex& v : verts)
        {
            v.m_position.x *= radius;
            v.m_position.y *= (height * 0.5f);
            v.m_position.z *= radius;
        }

        const MaterialPBR material { .m_base_color_factor = color };
        m_mesh_vector.emplace_back(std::move(verts), std::vector<GLuint>(instance.m_indices), std::make_shared<MaterialPBR>(material));
        
        m_position = position;
        m_rotation = rotation;
        m_aabb_half_extents = glm::vec3(radius, height * 0.5f, radius);
        CalculateAABBExtentsAndLocalCenter();
    }

    CylinderModel::CylinderModel(float radius, float height, int up_axis, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept
        : m_radius(radius) , m_height(height) , m_color(color)
    {
        const SimpleCylinder_r1_h2& instance = SimpleCylinder_r1_h2::GetInstance();
        std::vector<Vertex> verts = instance.m_verts;

        for (Vertex& v : verts)
        {
            v.m_position.x *= radius;
            v.m_position.y *= height * 0.5f;
            v.m_position.z *= radius;
        }

        const MaterialPBR material{.m_base_color_factor = color};

        m_mesh_vector.emplace_back(std::move(verts), std::vector<GLuint>(instance.m_indices), std::make_shared<MaterialPBR>(material));

        m_position = position;

        glm::quat axis_rotation = glm::identity<glm::quat>();

        switch (up_axis)
        {
            case 0:
                axis_rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
                break;

            case 1:
                break;

            case 2: 
                axis_rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
                break;

            default:
                break;
        }

        m_rotation = rotation * axis_rotation;

        m_aabb_half_extents = glm::vec3(radius, height * 0.5f, radius);

        CalculateAABBExtentsAndLocalCenter();
    }

    std::unique_ptr<Basic_Model> CylinderModel::Copy() const noexcept 
    {
        return std::make_unique<CylinderModel>(*this);
    }

    Basic_Model::ModelType CylinderModel::GetModelType() const noexcept 
    {
        return Basic_Model::ModelType::CYLINDER_MODEL;
    }

    float CylinderModel::GetUnscaledRadius() const noexcept
    {
        return m_radius;
    }

    float CylinderModel::GetHeight() const noexcept
    {
        return m_height;
    }

    glm::vec3 CylinderModel::GetColor() const noexcept
    {
        return m_color;
    }

}