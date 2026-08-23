#pragma once

#include "core/model/Model.h"

namespace CoreEngine
{
    class CylinderModel : public Basic_Model
    {
    private:
        float m_radius;
        float m_height;
        glm::vec3 m_color;

    public:
        explicit CylinderModel(float radius, float cylinder_height, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept;
        explicit CylinderModel(float radius, float height, int up_axis, glm::vec3 position, glm::quat rotation, glm::vec3 color) noexcept;

////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Basic_Model Abstract Methods
////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] virtual std::unique_ptr<Basic_Model> Copy() const noexcept override;
        [[nodiscard]] virtual Basic_Model::ModelType GetModelType() const noexcept override;

////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Own methods
////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] float GetUnscaledRadius() const noexcept;
        [[nodiscard]] float GetHeight() const noexcept;
        [[nodiscard]] glm::vec3 GetColor() const noexcept;
    };


}