#include "core/scene/Scene3D.h"

//std
#include <utility>
#include <filesystem>
#include <fstream>
#include <iostream>

//Own includes
#include "Scene3D_SerializeConstants.h"

#include "core/model/Model.h"

#include "core/physics/PhysicsCar.h"

#include "core/utility/PhysicsUtility.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/Assert.h"

namespace CoreEngine
{
////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Main update
////////////////////////////////////////////////////////////////////////////////////////////////////
    void Scene3D::OnPhysicsUpdate(const Units::MicroSecond delta_time) noexcept
    {
        m_physics_world.OnUpdate(delta_time);

        for (std::unique_ptr<Scene3D_SceneObject>& object : m_scene_objects)
        {
            object->SyncRenderTransformWithPhysics();
        }
    }

    void Scene3D::OnDrawBtDebug() noexcept
    {
        m_physics_world.GetDynamicsWorldPtr()->debugDrawWorld();
    }

    void Scene3D::SetDebugDrawer(btIDebugDraw* drawer) noexcept
    {
        m_physics_world.GetDynamicsWorldPtr()->setDebugDrawer(drawer);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Get scene / draw data
////////////////////////////////////////////////////////////////////////////////////////////////////
    const std::vector<Basic_Model*> Scene3D::GetRenderModelVector() const noexcept
    {   
        std::vector<Basic_Model*> models;
        models.reserve(m_scene_objects.size());

        for (const std::unique_ptr<Scene3D_SceneObject>& scene_obj : m_scene_objects)
        {
            if (scene_obj->m_render_model)
                models.push_back(scene_obj->m_render_model.get());
        }

        return models;
    }

    const std::vector<Light>& Scene3D::GetLightVectorConstRef() const noexcept
    {
        return m_light_sources;
    }

    std::vector<glm::vec3> Scene3D::GetDebugLinesAllObjects() const noexcept
    {
        std::vector<glm::vec3> lines_out;
        lines_out.reserve(m_scene_objects.size() * 12); //12 for rectangle prism, improve this guess
        
        for (const auto& obj : m_scene_objects)
        {
            const std::vector<glm::vec3>& lines_obj = obj->CalculateAABBDebugLines();
            lines_out.insert(lines_out.end(), lines_obj.begin(), lines_obj.end());
        }
        return lines_out;
    }

    std::vector<std::unique_ptr<Scene3D_SceneObject>>& Scene3D::GetSceneObjectsRef() noexcept
    {
        return m_scene_objects;
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Adding / Removing objects
////////////////////////////////////////////////////////////////////////////////////////////////////
    void Scene3D::AddObject(std::unique_ptr<Scene3D_SceneObject> obj) noexcept
    {
        m_scene_objects.push_back(std::move(obj));
        m_object_added_or_deleted = true;
    }

    Scene3D_ObjectBuilder Scene3D::CreateObjectBuilder() noexcept
    {
        return Scene3D_ObjectBuilder {m_physics_world.GetDynamicsWorldPtr()};
    }

    void Scene3D::AddObjectFromBuilder(Scene3D_ObjectBuilder&& builder) noexcept
    {
        m_scene_objects.push_back(builder.Finalize());
        m_object_added_or_deleted = true;
    }

    bool Scene3D::RemoveObject(const Scene3D_SceneObject* object_ptr) noexcept
    {
        const bool removed = std::erase_if(m_scene_objects, [object_ptr](const auto& object) -> bool { return object.get() == object_ptr; });
        if (removed)
        {
            m_object_added_or_deleted = true;
        }
        return removed;
    }

    bool Scene3D::RemoveObject(const std::size_t index) noexcept
    {
        if (index >= m_scene_objects.size())
            return false;
        m_scene_objects.erase(m_scene_objects.begin() + index);
        m_object_added_or_deleted = true;
        return true;
    }

    bool Scene3D::RemoveLightSource(const std::size_t index) noexcept
    { 
        if (index >= m_light_sources.size())
            return false;
        m_light_sources.erase(m_light_sources.begin() + index);
        m_light_added_or_deleted = true;
        return true;
    }

    bool Scene3D::GetAndResetObjectVecChangeFlag() noexcept
    {
        return std::exchange(m_object_added_or_deleted, false);
    }

    bool Scene3D::GetAndResetLightVecChangeFlag() noexcept
    {
        return std::exchange(m_light_added_or_deleted, false);
    }

    void Scene3D::ClearAllLightSources() noexcept
    {
        m_light_sources.clear();
        m_light_added_or_deleted = true;
    }

    void Scene3D::ClearAllSceneObjects() noexcept
    {
        m_scene_objects.clear();
        m_object_added_or_deleted = true;
    }

    void Scene3D::ClearAll() noexcept
    {
        m_scene_objects.clear();
        m_light_sources.clear();
        btIDebugDraw* debug_draw = m_physics_world.GetDynamicsWorldPtr()->getDebugDrawer();
        m_physics_world = PhysicsWorld();
        SetDebugDrawer(debug_draw);

        m_object_added_or_deleted = true;
        m_light_added_or_deleted  = true;
    }

    size_t Scene3D::GetAmountObjects() noexcept
    {
        return m_scene_objects.size();
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Serialization
////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Scene3D::SerializeToFile(const std::string& file_path, const CameraReverseZ& camera) const
    {
        std::ofstream file(file_path);

        if (!file.is_open()) { return false; }

        file << SerializeToString(camera);
        file.close();
        return true;
    }

    std::string Scene3D::SerializeToString(const CameraReverseZ& camera) const
    {
        nlohmann::ordered_json json_obj;

        json_obj[_SerializeKey::Meta::Root][_SerializeKey::Meta::Version]       = _SerializeKey::Meta::Value::ManifestVersion;
        json_obj[_SerializeKey::Meta::Root][_SerializeKey::Meta::TimestampNow]  = std::time(nullptr);

        json_obj[_SerializeKey::SceneInfo::Root][_SerializeKey::SceneInfo::AmountObjects] = m_scene_objects.size();
        json_obj[_SerializeKey::SceneInfo::Root][_SerializeKey::SceneInfo::AmountLights]  =  m_light_sources.size();
        
        ///////////////////////////////////////////////////
        //-------- Camera
        ///////////////////////////////////////////////////
        {
            auto& cam_root = json_obj[_SerializeKey::Camera::Root];
            const glm::vec3 c_pos = camera.GetPosition();
            const glm::quat c_rot = camera.GetRotation();
            cam_root[_SerializeKey::Camera::Position]  = {{"x", c_pos.x}, {"y", c_pos.y}, {"z", c_pos.z}};
            cam_root[_SerializeKey::Camera::Rotation]  = {{"w", c_rot.w}, {"x", c_rot.x}, {"y", c_rot.y}, {"z", c_rot.z}};
            cam_root[_SerializeKey::Camera::NearPlane] = camera.GetNearPlane();
            cam_root[_SerializeKey::Camera::FovDeg]    = camera.GetFovDeg();
        }

        ///////////////////////////////////////////////////
        //-------- Objects
        ///////////////////////////////////////////////////
        json_obj[_SerializeKey::SceneData::Root][_SerializeKey::SceneData::SceneObjects]  = nlohmann::json::array();
        for (const auto& object : m_scene_objects) 
        {
            json_obj[_SerializeKey::SceneData::Root][_SerializeKey::SceneData::SceneObjects].push_back(object->SerializeToJson());
        }

        ///////////////////////////////////////////////////
        //-------- Light Sources
        ///////////////////////////////////////////////////
        json_obj[_SerializeKey::SceneData::Root][_SerializeKey::SceneData::LightSources]  = nlohmann::json::array();
        for (const auto& light : m_light_sources)
        {
            json_obj[_SerializeKey::SceneData::Root][_SerializeKey::SceneData::LightSources].push_back(
                {
                    {_SerializeKey::Light::Position,   {{"x", light.m_position.x}, {"y", light.m_position.y}, {"z", light.m_position.z}}},
                    {_SerializeKey::Light::Color,      {{"r", light.m_color.r},    {"g", light.m_color.g},    {"b", light.m_color.b}}},
                    {_SerializeKey::Light::Intensity,  light.m_intensity},
                    {_SerializeKey::Light::Mode,       light.m_light_mode}
                }
            );
        }

        return json_obj.dump(0);
    }

    void Scene3D::LoadFromSerializedFile(const std::string& file_path, std::optional<CameraReverseZ*> opt_camera)
    {
        return LoadFromSerializedString(CommonUtility::ReadFileToString(file_path.c_str()), opt_camera);
    }

    void Scene3D::LoadFromSerializedString(const std::string& data, std::optional<CameraReverseZ*> opt_camera)
    {
        nlohmann::json json = nlohmann::json::parse(data);
        
        ClearAll();

        {
            const auto& scene_info_root_json = json.at(_SerializeKey::SceneInfo::Root);
            m_scene_objects.reserve(scene_info_root_json.at(_SerializeKey::SceneInfo::AmountObjects).get<size_t>());
            m_light_sources.reserve(scene_info_root_json.at(_SerializeKey::SceneInfo::AmountLights).get<size_t>());
        }

        const auto& scene_data_root_json = json.at(_SerializeKey::SceneData::Root);

        ///////////////////////////////////////////////////
        //-------- Scene Objects
        ///////////////////////////////////////////////////
        {
            const auto& scene_objects_json   = scene_data_root_json.at(_SerializeKey::SceneData::SceneObjects);
            for (const auto& object_json : scene_objects_json)
            {
                m_scene_objects.push_back(std::make_unique<Scene3D_SceneObject>(object_json, m_physics_world.GetDynamicsWorldPtr()));
            }
            
            m_object_added_or_deleted = true;
        }
        
        ///////////////////////////////////////////////////
        //-------- Light Sources
        ///////////////////////////////////////////////////
        {
            const auto& light_sources_json   = scene_data_root_json.at(_SerializeKey::SceneData::LightSources);
            for (const auto& light_json : light_sources_json)
            {
                const auto& color_json    = light_json.at(_SerializeKey::Light::Color);
                const auto& position_json = light_json.at(_SerializeKey::Light::Position);

                const glm::vec3 color     { color_json.at("r").get<float>(),    color_json.at("g").get<float>(),    color_json.at("b").get<float>()    };
                const glm::vec3 position  { position_json.at("x").get<float>(), position_json.at("y").get<float>(), position_json.at("z").get<float>() };
                const GLfloat   intensity = light_json.at(_SerializeKey::Light::Intensity).get<GLfloat>();
                const GLuint    mode      = light_json.at(_SerializeKey::Light::Mode).get<GLuint>();

                EmplaceLightSource(position, color, intensity, mode);
            }
            m_light_added_or_deleted  = true;
        }

        ///////////////////////////////////////////////////
        //-------- Camera
        ///////////////////////////////////////////////////
        if (opt_camera.has_value() && opt_camera.value())
        {
            const auto& camera_json = json.at(_SerializeKey::Camera::Root);
            const auto& c_pos_json  = camera_json.at(_SerializeKey::Camera::Position);
            const auto& c_rot_json  = camera_json.at(_SerializeKey::Camera::Rotation);

            const glm::vec3 c_pos { c_pos_json.at("x").get<float>(), c_pos_json.at("y").get<float>(), c_pos_json.at("z").get<float>() };
            const glm::quat c_rot { c_rot_json.at("w").get<float>(), c_rot_json.at("x").get<float>(), c_rot_json.at("y").get<float>(), c_rot_json.at("z").get<float>()};
            const float fov_deg = camera_json.at(_SerializeKey::Camera::FovDeg).get<float>();
            const float near    = camera_json.at(_SerializeKey::Camera::NearPlane).get<float>();

            CameraReverseZ* camera = *opt_camera;
            camera->SetPosition(c_pos);
            camera->SetRotation(c_rot);
            camera->SetFovDeg(fov_deg);
            camera->SetNearPlane(near);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //-------- Raycasting
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Scene3D::RaycastHit Scene3D::RaycastSelect(const MathUtility::Ray3D& ray) noexcept
    {
        RaycastHit closest_hit {};
        float closest_dist_along_ray = std::numeric_limits<float>::max();

        for (size_t scene_obj_index {0}; scene_obj_index < m_scene_objects.size(); ++scene_obj_index)
        {
            const Scene3D_SceneObject& object = *m_scene_objects[scene_obj_index];

            if (! object.m_render_model) continue;
            const Basic_Model* model = object.m_render_model.get();

            const glm::mat4 model_matrix     = model->GetModelMatrix();
            const glm::mat4 inv_model_matrix = glm::inverse(model_matrix);

            MathUtility::Ray3D model_space_ray;
            model_space_ray.m_origin               = glm::vec3(inv_model_matrix * glm::vec4(ray.m_origin, 1.0f));
            model_space_ray.m_direction_normalized = glm::normalize(glm::vec3(inv_model_matrix * glm::vec4(ray.m_direction_normalized, 0.0f)));
            model_space_ray.m_max_ray_length       = ray.m_max_ray_length;

            const MathUtility::AABB local_space_aabb = model->GetLocalSpaceAABB();

            ///////////////////////////////////////////////////
            //-------- Sphere Test
            ///////////////////////////////////////////////////
            {
                const float bounding_sphere_radius = model->GetLocalSpaceMaximumBoundingSphereRadius();
                const MathUtility::Sphere sphere {local_space_aabb.m_center, bounding_sphere_radius};
                const MathUtility::Raytest3D_Result sphere_test = MathUtility::RaySphereIntersect(model_space_ray, sphere);
                const float sphere_dist_along_ray = glm::dot(sphere_test.m_intersection_point - model_space_ray.m_origin, model_space_ray.m_direction_normalized);
        
                if (!sphere_test.m_has_hit || sphere_dist_along_ray >= closest_dist_along_ray || sphere_dist_along_ray > model_space_ray.m_max_ray_length) 
                {
                    continue;
                }
            }

            ///////////////////////////////////////////////////
            //-------- AABB Test
            ///////////////////////////////////////////////////
            {
                const MathUtility::Raytest3D_Result aabb_test = MathUtility::RayAABBIntersect(model_space_ray, local_space_aabb);
                const float aabb_dist_along_ray = glm::dot(aabb_test.m_intersection_point - model_space_ray.m_origin, model_space_ray.m_direction_normalized);

                if (!aabb_test.m_has_hit || aabb_dist_along_ray >= closest_dist_along_ray || aabb_dist_along_ray > model_space_ray.m_max_ray_length) 
                {
                    continue;
                }
            }

            for (const Mesh& mesh : model->GetMeshVectorConstReference())
            {
                ///////////////////////////////////////////////////
                //-------- AABB Test per mesh
                ///////////////////////////////////////////////////
                {
                    const MathUtility::AABB mesh_model_space_aabb (mesh.GetLocalAABBHalfExtents(), mesh.GetLocalCenter());
                    const MathUtility::Raytest3D_Result aabb_test = MathUtility::RayAABBIntersect(model_space_ray, mesh_model_space_aabb);
                    const float aabb_dist_along_ray = glm::dot(aabb_test.m_intersection_point - model_space_ray.m_origin, model_space_ray.m_direction_normalized);

                    if (!aabb_test.m_has_hit || aabb_dist_along_ray >= closest_dist_along_ray || aabb_dist_along_ray > model_space_ray.m_max_ray_length) 
                    {
                        continue;
                    }
                }

                ///////////////////////////////////////////////////
                //-------- Per triangle test
                ///////////////////////////////////////////////////
                const std::vector<Vertex>& vertices = mesh.GetVerticesConstReference();
                const std::vector<GLuint>& indices  = mesh.GetIndicesConstReference();

                ENGINE_ASSERT (indices.size() % 3 == 0 && "At Scene3D::RaycastSelect(): Meshes must be triangulated.");
                for (size_t triangle_indices = 0; triangle_indices <indices.size(); triangle_indices += 3)
                {
                    const glm::vec3 vert0 = vertices[indices[triangle_indices + 0]].m_position;
                    const glm::vec3 vert1 = vertices[indices[triangle_indices + 1]].m_position;
                    const glm::vec3 vert2 = vertices[indices[triangle_indices + 2]].m_position;

                    const MathUtility::Raytest3D_Result result = MathUtility::RayTriangleIntersect(model_space_ray, MathUtility::Triangle {vert0, vert1, vert2} );
                    if (! result.m_has_hit) continue;
                    
                    const float triangle_dist_along_ray = glm::dot(result.m_intersection_point - model_space_ray.m_origin, model_space_ray.m_direction_normalized);
                    if (closest_dist_along_ray < triangle_dist_along_ray) continue;

                    closest_hit.m_scene_object_ptr   = m_scene_objects[scene_obj_index].get();
                    closest_hit.m_intersection_point = glm::vec3(model_matrix * glm::vec4(result.m_intersection_point, 1.0f));

                    glm::vec3 normal = glm::normalize(glm::cross(vert1 - vert0, vert2 - vert0));
                    if (glm::dot(normal, model_space_ray.m_direction_normalized) > 0)
                        normal = -normal;

                    closest_hit.m_normal = normal;

                    closest_dist_along_ray = triangle_dist_along_ray;
                }
            }
        }

        if (closest_dist_along_ray < ray.m_max_ray_length)
            return closest_hit;

        return { nullptr };
    }
}