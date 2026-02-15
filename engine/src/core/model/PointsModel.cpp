#include "core/model/PointsModel.h"

#include <numeric>
#include <memory>

namespace CoreEngine
{
    PointsModel::PointsModel(std::vector<Mesh>&& meshes, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& color) noexcept
    : m_color(color)
    {
        m_mesh_vector  = std::move(meshes);
        m_position     = position;
        m_rotation     = rotation;
        
        CalculateAABBExtentsAndLocalCenter();
        CenterModelLocally();
    }

    std::unique_ptr<Basic_Model> PointsModel::Copy() const noexcept
    {
        return std::make_unique<PointsModel>(*this);
    }

    Basic_Model::ModelType PointsModel::GetModelType() const noexcept
    {
        return Basic_Model::ModelType::POINTS_MODEL;
    }

    glm::vec3 PointsModel::GetColor() const noexcept
    {
        return m_color;
    }
}