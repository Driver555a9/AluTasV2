#include "core/model/PathModel.h"

//Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//Standard includes
#include <filesystem>
#include <iostream>

//own
#include "core/utility/Assert.h"
#include "core/utility/CommonUtility.h"

namespace CoreEngine
{
    PathModel::PathModel(const std::string& path, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& natural_scale) noexcept 
    : m_file_path(path), m_natural_scale_factor(natural_scale)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
        
        ENGINE_ASSERT (scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode 
            && (std::string("At PathModel::PathModel(): Assimp error: ") + importer.GetErrorString()).c_str() );

        m_mesh_vector.reserve(scene->mNumMeshes);

        for (unsigned int i = 0; i < scene->mNumMeshes; i++) 
        {
            if (scene->mMeshes[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue;
            m_mesh_vector.emplace_back( GetMeshFromAi(path, scene->mMeshes[i], scene, natural_scale) );
        }

        m_position = position;
        m_rotation = rotation;
        
        CalculateAABBExtentsAndLocalCenter();
        CenterModelLocally();
    }

    std::unique_ptr<Basic_Model> PathModel::Copy() const noexcept
    {
        return std::make_unique<PathModel>(*this);
    }

    Basic_Model::ModelType PathModel::GetModelType() const noexcept
    {
        return Basic_Model::ModelType::PATH_MODEL;
    }

    std::string PathModel::GetFilePath() const noexcept
    {
        return m_file_path;
    }

    glm::vec3 PathModel::GetNaturalScaleFactor() const noexcept
    {
        return m_natural_scale_factor;
    }

    Mesh PathModel::GetMeshFromAi(const std::string& model_file_path, const aiMesh* mesh, const aiScene* scene, const glm::vec3& natural_scale) noexcept
    {
        ENGINE_ASSERT(mesh && scene && "At PathModel::GetMeshFromAi(): Mesh and Scene must not be nullptr");
        //////////////////////////////////////////////// 
        //---------  Extract geometry
        //////////////////////////////////////////////// 
        std::vector<Vertex> vertices;
        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex v;
            v.m_position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z) * natural_scale;

            v.m_normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);

            if (mesh->HasTextureCoords(0))
                v.m_tex_uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                v.m_tex_uv = glm::vec2(0.0f);

            vertices.push_back(v);
        }

        std::vector<uint32_t> indices;
        indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        /////////////////////////////////////////////// 
        //--------- Load Material
        //////////////////////////////////////////////// 
        std::shared_ptr<MaterialPBR> material;

        if (mesh->mMaterialIndex >= 0)
        {
            const aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
            material = ExtractMaterial(model_file_path, aiMat, scene);
        }
        else
        {
            material = std::make_shared<MaterialPBR>();
        }

        /////////////////////////////////////////////// 
        //--------- Load Material
        ////////////////////////////////////////////////
        return Mesh{ std::move(vertices), std::move(indices), material };
    }

    std::shared_ptr<MaterialPBR> PathModel::ExtractMaterial(const std::string& model_file_path, const aiMaterial* aiMat, const aiScene* scene) noexcept
    {
        ENGINE_ASSERT(aiMat && scene && "At PathModel::ExtractMaterial(): Material and Scene must not be nullptr");
        std::shared_ptr<MaterialPBR> material = std::make_shared<MaterialPBR>();

        /////////////////////////////////////////////// 
        //--------- Extract factors
        ////////////////////////////////////////////////
        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
        {
            material->m_base_color_factor = glm::vec3(color.r, color.g, color.b);
        }

        aiMat->Get(AI_MATKEY_METALLIC_FACTOR, material->m_metallic_factor);

        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive))
        {
            material->m_emissive_factor = glm::vec3(emissive.r, emissive.g, emissive.b);
        }

        aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, material->m_roughness_factor);

        /////////////////////////////////////////////// 
        //--------- Load Textures
        ////////////////////////////////////////////////
        const auto LoadTexture = [&](aiTextureType type) -> std::shared_ptr<Texture> 
        {
            if (aiMat->GetTextureCount(type) <= 0)
            {
                return nullptr;
            }

            aiString path;
            if (AI_SUCCESS != aiMat->GetTexture(type, 0, &path))
            {
                return nullptr;
            }

            const char* local_path = path.C_Str();

            if (local_path[0] == '*') 
            {
                const int texIndex = atoi(local_path + 1);
                if (texIndex >= 0 && texIndex < static_cast<int>(scene->mNumTextures))
                {
                    const aiTexture* embedded_tex = scene->mTextures[texIndex];
                    return std::make_shared<Texture>(embedded_tex);
                }
                return nullptr;
            }

            const std::filesystem::path directory = std::filesystem::path(model_file_path).parent_path();
            const std::filesystem::path fullPath  = directory / local_path;
            if (std::filesystem::exists(fullPath))
            {
                return std::make_shared<Texture>(fullPath.string().c_str());
            }

            return nullptr;
        };

        material->m_base_texture               = LoadTexture(aiTextureType_BASE_COLOR);
        if (!material->m_base_texture)
        {
            material->m_base_texture           = LoadTexture(aiTextureType_DIFFUSE);
        }

        material->m_metallic_roughness_texture = LoadTexture(aiTextureType_METALNESS);
        if (!material->m_metallic_roughness_texture)
        {
            material->m_metallic_roughness_texture  = LoadTexture(aiTextureType_DIFFUSE_ROUGHNESS);
        }

        material->m_normal_texture                  = LoadTexture(aiTextureType_NORMALS);
        material->m_occlusion_texture               = LoadTexture(aiTextureType_LIGHTMAP);
        material->m_emissive_texture                = LoadTexture(aiTextureType_EMISSIVE);

        return material;
    }
}