#pragma once

#include "core/model/Model.h"

namespace CoreEngine
{
    class SphereModel : public Basic_Model
    {
    private:
        float m_radius;
        glm::vec3 m_color;

    public:
        explicit SphereModel(float radius, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept;

////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Basic_Model Abstract Methods
////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] virtual std::unique_ptr<Basic_Model> Copy() const noexcept override;
        [[nodiscard]] virtual Basic_Model::ModelType GetModelType() const noexcept override;

////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Own methods
////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] float GetUnscaledRadius() const noexcept;
        [[nodiscard]] glm::vec3 GetColor() const noexcept;
    };


}