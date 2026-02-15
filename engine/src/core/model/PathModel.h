#pragma once

#include "core/model/Model.h"

namespace CoreEngine
{
    class PathModel : public Basic_Model
    {
    public:
        explicit PathModel(const std::string& path, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& natural_scale) noexcept;
        
        /////////////////////////////////////////////// 
        //--------- Basic Model Abstract methods
        //////////////////////////////////////////////// 
        [[nodiscard]] virtual std::unique_ptr<Basic_Model> Copy() const noexcept override;
        [[nodiscard]] virtual Basic_Model::ModelType GetModelType() const noexcept override;

        /////////////////////////////////////////////// 
        //--------- Own methods
        //////////////////////////////////////////////// 
        [[nodiscard]] std::string GetFilePath() const noexcept;
        [[nodiscard]] glm::vec3 GetNaturalScaleFactor() const noexcept;

    protected:
        std::string m_file_path;
        glm::vec3   m_natural_scale_factor;

        [[nodiscard]] static Mesh GetMeshFromAi(const std::string& model_file_path, const aiMesh* mesh, const aiScene* scene, const glm::vec3& natural_scale) noexcept;
        [[nodiscard]] static std::shared_ptr<MaterialPBR> ExtractMaterial(const std::string& model_file_path, const aiMaterial* mesh, const aiScene* scene) noexcept;
    };

}