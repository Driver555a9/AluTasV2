#pragma once

#include "core/model/Model.h"

namespace CoreEngine
{

    class PointsModel : public Basic_Model
    {
    private:
        glm::vec3 m_color;

    public:
        explicit PointsModel(std::vector<Mesh>&& meshes, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& color) noexcept;
        
////////////////////////////////////////////////////////////////////////////////////////////////
        //Basic_Model Abstract Methods
        [[nodiscard]] virtual std::unique_ptr<Basic_Model> Copy() const noexcept override;
        [[nodiscard]] virtual Basic_Model::ModelType GetModelType() const noexcept override;

////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] glm::vec3 GetColor() const noexcept;
    };

}