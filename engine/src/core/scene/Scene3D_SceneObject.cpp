#include "core/scene/Scene3D_SceneObject.h"

//Own includes
#include "Scene3D_SerializeConstants.h"

#include "core/model/BoxModel.h"
#include "core/model/SphereModel.h"
#include "core/model/PathModel.h"
#include "core/model/PointsModel.h"

#include "core/physics/PhysicsCar.h"

#include "core/utility/PhysicsUtility.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/Assert.h"

namespace CoreEngine
{
////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Construct from existing model and physics object
////////////////////////////////////////////////////////////////////////////////////////////////////
    Scene3D_SceneObject::Scene3D_SceneObject(std::unique_ptr<Basic_Model> model, std::unique_ptr<PhysicsObject> physics_obj, std::string&& name) noexcept
    : m_render_model(std::move(model)), m_physics_object(std::move(physics_obj)), m_name(std::move(name)) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Copy Construct
////////////////////////////////////////////////////////////////////////////////////////////////////
    Scene3D_SceneObject::Scene3D_SceneObject(const Scene3D_SceneObject& other) noexcept
    {
        if (other.m_render_model)
        {
            m_render_model = other.m_render_model->Copy();
        }
        if (other.m_physics_object)
        {
            m_physics_object = other.m_physics_object->Copy();
        }
        m_name = other.m_name;
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Construct from serialized json 
////////////////////////////////////////////////////////////////////////////////////////////////////
    Scene3D_SceneObject::Scene3D_SceneObject(const nlohmann::ordered_json& json, btDiscreteDynamicsWorld* physics_world) noexcept
    {
        m_name = json.at(_SerializeKey::SceneObject::Name).get<std::string>();
        if (json.contains(_SerializeKey::Render::Root))
        {
            const auto& render_json = json.at(_SerializeKey::Render::Root);

            const auto& pos_json = render_json.at(_SerializeKey::Render::Position);
            const auto& rot_json = render_json.at(_SerializeKey::Render::Rotation);
            const glm::vec3 position { pos_json.at("x").get<float>(), pos_json.at("y").get<float>(), pos_json.at("z").get<float>() };
            const glm::quat rotation { rot_json.at("w").get<float>(), rot_json.at("x").get<float>(), rot_json.at("y").get<float>(), rot_json.at("z").get<float>() };
            const std::string model_type = render_json.at(_SerializeKey::Render::Type).get<std::string>();
            
            const auto& scale_json      = render_json.at(_SerializeKey::Render::Scale);
            const glm::vec3 scale       { scale_json.at("x").get<float>(), scale_json.at("y").get<float>(), scale_json.at("z").get<float>() };

            if (model_type == _SerializeKey::Render::Value::IsFilePathModel)
            {
                const std::string path         = render_json.at(_SerializeKey::Render::FilePath).get<std::string>();
                const auto& natural_scale_json = render_json.at(_SerializeKey::Render::NaturalScale);
                const glm::vec3 natural_scale  { natural_scale_json.at("x").get<float>(), natural_scale_json.at("y").get<float>(), natural_scale_json.at("z").get<float>() };
                m_render_model = std::make_unique<PathModel>(path, position, rotation, natural_scale);
                m_render_model->SetScale(scale);
            }
            else if (model_type == _SerializeKey::Render::Value::IsBoxModel)
            {
                const auto& color_json = render_json.at(_SerializeKey::Render::PrimitiveColor);
                const glm::vec3 color  { color_json.at("r").get<float>(), color_json.at("g").get<float>(), color_json.at("b").get<float>() };

                const auto& half_extents_json = render_json.at(_SerializeKey::Render::HalfExtents);
                const glm::vec3 half_extents  { half_extents_json.at("x").get<float>(), half_extents_json.at("y").get<float>(), half_extents_json.at("z").get<float>() };
                
                m_render_model = std::make_unique<BoxModel>(half_extents * 2.0f, position, rotation, color);
                m_render_model->SetScale(scale);
            }
            else if (model_type == _SerializeKey::Render::Value::IsSphereModel)
            {
                const auto& color_json = render_json.at(_SerializeKey::Render::PrimitiveColor);
                const glm::vec3 color  { color_json.at("r").get<float>(), color_json.at("g").get<float>(), color_json.at("b").get<float>() };

                const float radius = render_json.at(_SerializeKey::Render::Radius).get<float>();

                m_render_model = std::make_unique<SphereModel>(radius, position, rotation, color);
                m_render_model->SetScale(scale);
            }
            else if (model_type == _SerializeKey::Render::Value::IsPointModel)
            {
                const auto& color_json = render_json.at(_SerializeKey::Render::PrimitiveColor);

                const glm::vec3 color  { color_json.at("r").get<float>(), color_json.at("g").get<float>(), color_json.at("b").get<float>() };

                const auto& meshes_json = render_json.at(_SerializeKey::Render::Meshes);

                std::vector<Mesh> meshes;
                meshes.reserve(meshes_json.size());
                
                for (const auto& mesh_json : meshes_json)
                {
                    std::vector<Vertex> vertices;
                    std::vector<GLuint> indices;

                    const auto& vertices_json = mesh_json.at(_SerializeKey::Render::MeshVertices);
                    vertices.reserve(vertices_json.size());

                    for (const auto& v : vertices_json)
                    {
                        Vertex vert;
                        vert.m_position  = { v[0], v[1], v[2] };
                        vert.m_normal    = { v[3], v[4], v[5] };
                        vert.m_tex_uv    = { v[6], v[7] };

                        vertices.push_back(vert);
                    }

                    const auto& indices_json = mesh_json.at(_SerializeKey::Render::MeshIndices);
                    indices.reserve(indices_json.size());

                    for (auto index : indices_json)
                    {
                        indices.push_back(index.get<GLuint>());
                    }

                    meshes.push_back(Mesh {std::move(vertices), std::move(indices), std::make_shared<MaterialPBR>(MaterialPBR{ .m_base_color_factor = color})});
                }

                m_render_model = std::make_unique<PointsModel>(std::move(meshes), position, rotation, color);
                m_render_model->SetScale(scale);
            }
            else { ENGINE_ASSERT (false && "At SceneObject(const nlohmann::json&): Failed to construct Object - Unkown type."); }
        }
        
        if (json.contains(_SerializeKey::Physics::Root)) 
        {
            const auto& physics_json = json.at(_SerializeKey::Physics::Root);

            const auto& pos_json = physics_json.at(_SerializeKey::Physics::Position);
            const auto& rot_json = physics_json.at(_SerializeKey::Physics::Rotation);
            const btVector3 position { pos_json.at("x").get<float>(), pos_json.at("y").get<float>(), pos_json.at("z").get<float>() };
            //bt is xyzw as opposed to glm wxyz !!
            const btQuaternion rotation { rot_json.at("x").get<float>(), rot_json.at("y").get<float>(), rot_json.at("z").get<float>(), rot_json.at("w").get<float>() };

            const std::string shape       = physics_json.at(_SerializeKey::Physics::Shape).get<std::string>();
            const std::string object_type = physics_json.at(_SerializeKey::Physics::ObjectType).get<std::string>();
            const auto& scale_json        = physics_json.at(_SerializeKey::Physics::Scale);  

            PhysicsObjectConfig config;
            config.m_mass  = physics_json.at(_SerializeKey::Physics::Mass).get<float>();

            config.m_start_transform.setOrigin(position);
            config.m_start_transform.setRotation(rotation);
            
            config.m_local_scaling = btVector3(scale_json.at("x").get<float>(), scale_json.at("y").get<float>(), scale_json.at("z").get<float>());

            if (shape == _SerializeKey::Physics::Value::Box)
            {
                config.m_shape_type          = PhysicsShapeType::BOX;
                const auto& half_extent_json = physics_json.at(_SerializeKey::Physics::BoxHalfExtents);
                config.m_box_half_extents    = btVector3(half_extent_json.at("x").get<float>(), half_extent_json.at("y").get<float>(), half_extent_json.at("z").get<float>());
            }
            else if (shape == _SerializeKey::Physics::Value::Sphere)
            {
                config.m_shape_type    = PhysicsShapeType::SPHERE;
                config.m_sphere_radius = physics_json.at(_SerializeKey::Physics::SphereRadius).get<float>();
            }
            else if (shape == _SerializeKey::Physics::Value::TriangleMesh)
            {
                std::shared_ptr<btTriangleMesh> triangle_mesh = std::make_unique<btTriangleMesh>();
                const auto& points_json = physics_json.at(_SerializeKey::Physics::MeshVertexPositions);

                for (size_t i = 0; i + 2 < points_json.size(); i += 3)
                {
                    const auto& p0 = points_json[i];
                    const auto& p1 = points_json[i + 1];
                    const auto& p2 = points_json[i + 2];

                    triangle_mesh->addTriangle(
                        btVector3(p0[0].get<float>(), p0[1].get<float>(), p0[2].get<float>()),
                        btVector3(p1[0].get<float>(), p1[1].get<float>(), p1[2].get<float>()),
                        btVector3(p2[0].get<float>(), p2[1].get<float>(), p2[2].get<float>())
                    );
                }

                config.m_shape_type = PhysicsShapeType::TRIANGLE_MESH_SHAPE;
                config.m_triangle_mesh_shape = std::make_unique<OwnedBvhTriangleMeshShape>(std::move(triangle_mesh), true);
            }
            else { ENGINE_ASSERT (false && "At SceneObject(const nlohmann::json&): json must provide a valid physics shape (e.g. box, sphere)."); }

            if (object_type == _SerializeKey::Physics::Value::ObjectTypeOrdinary)
            {
                m_physics_object = std::make_unique<PhysicsObject>(physics_world, std::move(config));
            }
            else if (object_type == _SerializeKey::Physics::Value::ObjectTypeCar)
            {
                m_physics_object = std::make_unique<PhysicsCar>(physics_world, std::move(config));
            }
            else { ENGINE_ASSERT (false && "At SceneObject(const nlohmann::json&): json must provide a valid object type (e.g. ordinary, car)."); }
        }
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Getters / Setters
////////////////////////////////////////////////////////////////////////////////////////////////////
    glm::vec3 Scene3D_SceneObject::GetPosition() const noexcept
    {
        if (m_render_model)
            return m_render_model->GetPosition();
        if (m_physics_object)
            return PhysicsUtility::BtVector3ToGlm(m_physics_object->GetBody()->getWorldTransform().getOrigin());
        return glm::vec3{};
    }

    void Scene3D_SceneObject::SetPosition(const glm::vec3& pos) noexcept
    {
        if (m_render_model)
        {
            m_render_model->SetPosition(pos);
        }
        if (m_physics_object)
        {
            btRigidBody* body = m_physics_object->GetBody();
            btTransform trans = body->getWorldTransform();
            
            btVector3 final_physics_pos = PhysicsUtility::GlmVec3ToBt(pos);

            if (m_physics_object->GetShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && 
                m_physics_object->GetShapeType() == PhysicsShapeType::OWNED_COMPOUND_SHAPE)
            {
                const OwnedCompoundShape* compound = static_cast<OwnedCompoundShape*>(m_physics_object->GetShape());
                const btVector3 local_offset   = compound->GetCenterOfMassOffset();
                const btVector3 rotated_offset = trans.getBasis() * local_offset;
                
                final_physics_pos -= rotated_offset;
            }

            trans.setOrigin(final_physics_pos);
            
            body->setWorldTransform(trans);
            body->setInterpolationWorldTransform(trans);
            m_physics_object->GetMotion()->setWorldTransform(trans);
            
            m_physics_object->ResetMotion();
            body->activate(true);
        }
    }

    glm::quat Scene3D_SceneObject::GetRotation() const noexcept
    {   
        if (m_render_model)
            return m_render_model->GetRotation();
        if (m_physics_object)
            return PhysicsUtility::BtQuatToGlm(m_physics_object->GetBody()->getWorldTransform().getRotation());
        return glm::quat{};
    }   

    void Scene3D_SceneObject::SetRotation(const glm::quat& rot) noexcept
    {
        if (m_render_model)
        {
            m_render_model->SetRotation(rot);
        }
        if (m_physics_object)
        {
            btTransform trans = m_physics_object->GetBody()->getWorldTransform();
            trans.setRotation(PhysicsUtility::GlmQuatToBt(rot));
            m_physics_object->GetMotion()->setWorldTransform(trans);
            m_physics_object->GetBody()->setWorldTransform(trans);
        }
    }

    glm::vec3 Scene3D_SceneObject::GetScale() const noexcept
    {
        if (m_render_model)
            return m_render_model->GetScale();
        if (m_physics_object)
            return PhysicsUtility::BtVector3ToGlm(m_physics_object->GetShape()->getLocalScaling());
        return glm::vec3{1.0f};
    }

    void Scene3D_SceneObject::SetScale(const glm::vec3& scale) noexcept
    {
        if (m_render_model)
        {
            m_render_model->SetScale(scale);
        }
        if (m_physics_object)
        {
            if (m_physics_object->GetShapeType() == PhysicsShapeType::SPHERE)
            {
                ENGINE_ASSERT (scale.x == scale.y && scale.y == scale.z && "At Scene3D_SceneObject::SetScale(): Non-uniform scaling not supported for spheres.");
                m_physics_object->GetShape()->setLocalScaling(btVector3(scale.x, scale.x, scale.x));
            }
            m_physics_object->GetShape()->setLocalScaling(PhysicsUtility::GlmVec3ToBt(scale));
        }
    }

    void Scene3D_SceneObject::SyncRenderTransformWithPhysics() noexcept
    {
        if (!m_render_model || !m_physics_object) return; 

        btTransform physics_transform;
        m_physics_object->GetMotion()->getWorldTransform(physics_transform);

        if (m_physics_object->GetShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && m_physics_object->GetShapeType() == PhysicsShapeType::OWNED_COMPOUND_SHAPE)
        {
            const btVector3 local_offset = static_cast<OwnedCompoundShape*>(m_physics_object->GetShape())->GetCenterOfMassOffset();

            btTransform offset_transform;
            offset_transform.setIdentity();
            offset_transform.setOrigin(local_offset);

            const btTransform visual_transform = physics_transform * offset_transform;

            m_render_model->SetPosition(PhysicsUtility::BtVector3ToGlm(visual_transform.getOrigin()));
            m_render_model->SetRotation(PhysicsUtility::BtQuatToGlm(visual_transform.getRotation()));
        }
        else 
        {
            m_render_model->SetPosition(PhysicsUtility::BtVector3ToGlm(physics_transform.getOrigin()));
            m_render_model->SetRotation(PhysicsUtility::BtQuatToGlm(physics_transform.getRotation()));
        }
    }

    float Scene3D_SceneObject::GetWorldSpaceMaxBoundingSphereRadius() const noexcept
    {
        float render_radius  {0.0f};
        float physics_radius {0.0f};

        if (m_render_model)
            render_radius = m_render_model->GetWorldSpaceMaximumBoundingSphereRadius();
        if (m_physics_object)
        {
            btVector3 center;
            m_physics_object->GetShape()->getBoundingSphere(center, physics_radius);
        }
        return std::max(render_radius, physics_radius);
    }

    bool Scene3D_SceneObject::HasEitherRenderModelOrPhysicsObject() const noexcept
    {
        return m_render_model || m_physics_object;
    }

    std::vector<glm::vec3> Scene3D_SceneObject::CalculateAABBDebugLines() const noexcept
    {
        const auto PushLine = [](std::vector<glm::vec3>& vec, const glm::vec3& a, const glm::vec3& b)
        {
            vec.push_back(a);
            vec.push_back(b);
        };

        const auto GenerateAABB_Vector = [&PushLine](const glm::vec3& min, const glm::vec3& max)
        {
            std::vector<glm::vec3> v;
            glm::vec3 c[8] = {
                {min.x, min.y, min.z},
                {max.x, min.y, min.z},
                {max.x, max.y, min.z},
                {min.x, max.y, min.z},

                {min.x, min.y, max.z},
                {max.x, min.y, max.z},
                {max.x, max.y, max.z},
                {min.x, max.y, max.z}
            };

            PushLine(v, c[0], c[1]); PushLine(v, c[1], c[2]);
            PushLine(v, c[2], c[3]); PushLine(v, c[3], c[0]);

            PushLine(v, c[4], c[5]); PushLine(v, c[5], c[6]);
            PushLine(v, c[6], c[7]); PushLine(v, c[7], c[4]);

            PushLine(v, c[0], c[4]); PushLine(v, c[1], c[5]);
            PushLine(v, c[2], c[6]); PushLine(v, c[3], c[7]);
            return v;
        };

        if (m_physics_object)
        {
            if (const btRigidBody* rigidBody = m_physics_object->GetBody())
            {
                btVector3 min_corner, max_corner;
                rigidBody->getAabb(min_corner, max_corner);
                return GenerateAABB_Vector(PhysicsUtility::BtVector3ToGlm(min_corner), PhysicsUtility::BtVector3ToGlm(max_corner));
            }
        }
        else if (m_render_model)
        {
            const MathUtility::AABB aabb = m_render_model->GetWorldSpaceAABB();
            return GenerateAABB_Vector(aabb.min(), aabb.max());
        }
        
        return {};
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
//-------- Serialization 
////////////////////////////////////////////////////////////////////////////////////////////////////
    [[nodiscard]] nlohmann::ordered_json Scene3D_SceneObject::SerializeToJson() const noexcept
    {
        nlohmann::json json_obj;

        json_obj[_SerializeKey::SceneObject::Name] = m_name;

        if (m_render_model)
        {
            const glm::vec3 position = m_render_model->GetPosition();
            const glm::quat rotation = m_render_model->GetRotation();
            json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Position] = {{"x", position.x}, {"y", position.y}, {"z", position.z}};
            json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Rotation] = {{"w", rotation.w}, {"x", rotation.x}, {"y", rotation.y}, {"z", rotation.z}};

            const glm::vec3 scale = m_render_model->GetScale();
            json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Scale]    = {{"x", scale.x}, {"y", scale.y}, {"z", scale.z}};

            if (m_render_model->GetModelType() == Basic_Model::ModelType::PATH_MODEL)
            {
                const PathModel* path_model   = static_cast<PathModel*>(m_render_model.get());
                const glm::vec3 natural_scale = path_model->GetNaturalScaleFactor(); // Computed once on CPU unpon load
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Type]         = _SerializeKey::Render::Value::IsFilePathModel;
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::FilePath]     = path_model->GetFilePath();
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::NaturalScale] = {{"x", natural_scale.x}, {"y", natural_scale.y}, {"z", natural_scale.z}};
            }
            else if (m_render_model->GetModelType() == Basic_Model::ModelType::PRIMITIVE_BOX)
            {
                const BoxModel* box = static_cast<BoxModel*>(m_render_model.get());
                const glm::vec3 half_extents = m_render_model->GetLocalAABBHalfExtents();
                const glm::vec3 primitive_color = box->GetColor();
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Type]           = _SerializeKey::Render::Value::IsBoxModel;
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::PrimitiveColor] = {{"r", primitive_color.x}, {"g", primitive_color.y}, {"b", primitive_color.z}};
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::HalfExtents]    = {{"x", half_extents.x}, {"y", half_extents.y}, {"z", half_extents.z}};
            }
            else if (m_render_model->GetModelType() == Basic_Model::ModelType::PRIMITIVE_SPHERE)
            {
                const SphereModel* sphere = static_cast<SphereModel*>(m_render_model.get());
                const glm::vec3 primitive_color = sphere->GetColor();
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Type]           = _SerializeKey::Render::Value::IsSphereModel;
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::PrimitiveColor] = {{"r", primitive_color.x}, {"g", primitive_color.y}, {"b", primitive_color.z}};
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Radius]         =  sphere->GetUnscaledRadius();
            }
            else if (m_render_model->GetModelType() == Basic_Model::ModelType::POINTS_MODEL)
            {
                const PointsModel* points_model = static_cast<PointsModel*>(m_render_model.get());
                const glm::vec3 primitive_color = points_model->GetColor();

                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Type]           = _SerializeKey::Render::Value::IsPointModel;
                json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::PrimitiveColor] = {{"r", primitive_color.x}, {"g", primitive_color.y}, {"b", primitive_color.z}};
                
                auto& meshes_json = json_obj[_SerializeKey::Render::Root][_SerializeKey::Render::Meshes];
                meshes_json = nlohmann::json::array();

                for (const Mesh& mesh : points_model->GetMeshVectorConstReference())
                {
                    meshes_json.push_back(nlohmann::json::object());
                    auto& mesh_json = meshes_json.back();

                    auto& vertices_json = mesh_json[_SerializeKey::Render::MeshVertices];
                    vertices_json = nlohmann::json::array();

                    auto& indices_json = mesh_json[_SerializeKey::Render::MeshIndices];
                    indices_json = nlohmann::json::array();

                    for (const Vertex& vert : mesh.GetVerticesConstReference())
                    {
                        vertices_json.emplace_back(nlohmann::json::array({
                            vert.m_position.x, vert.m_position.y, vert.m_position.z,
                            vert.m_normal.x,   vert.m_normal.y,   vert.m_normal.z,
                            vert.m_tex_uv.x,   vert.m_tex_uv.y
                        }));
                    }

                    for (GLuint index : mesh.GetIndicesConstReference())
                    {
                        indices_json.push_back(index);
                    }
                }
            }
            else { ENGINE_ASSERT (false && "At SceneObject::SerializeToJson(): Invalid primitive provided by model."); }
        }

        if (m_physics_object)
        {
            const btTransform trans = m_physics_object->GetBody()->getWorldTransform();
            const btVector3 pos     = trans.getOrigin();
            const btQuaternion rot  = trans.getRotation();

            json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Position] = {{"x", pos.x()}, {"y", pos.y()}, {"z", pos.z()}};
            json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Rotation] = {{"x", rot.x()}, {"y", rot.y()}, {"z", rot.z()}, {"w", rot.w()}};
            
            json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Mass] = m_physics_object->GetBody()->getMass();
            const btVector3 scale = m_physics_object->GetShape()->getLocalScaling();
            json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Scale] = {{"x", scale.x()}, {"y", scale.y()}, {"z", scale.z()}};

            m_physics_object->GetShape()->setLocalScaling(btVector3(1, 1, 1)); // Temporarily remove scaling

            //////////////////////////////////////////////// 
            //--------- Save shape
            //////////////////////////////////////////////// 
            PhysicsShapeType shape_type = m_physics_object->GetShapeType();

            if (shape_type == PhysicsShapeType::BOX)
            {
                const btBoxShape* shape = static_cast<btBoxShape*>(m_physics_object->GetShape());
                const btVector3 half_extents = shape->getHalfExtentsWithoutMargin();
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Shape]          = _SerializeKey::Physics::Value::Box;
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::BoxHalfExtents] = {{"x", half_extents.x()}, {"y", half_extents.y()}, {"z", half_extents.z()}};
            }
            else if (shape_type == PhysicsShapeType::SPHERE)
            {
                const btSphereShape* shape = static_cast<btSphereShape*>(m_physics_object->GetShape());
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Shape]        = _SerializeKey::Physics::Value::Sphere;
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::SphereRadius] = shape->getRadius();
            }
            else if (shape_type == PhysicsShapeType::TRIANGLE_MESH_SHAPE)
            {
                const OwnedBvhTriangleMeshShape* triangle_shape = static_cast<OwnedBvhTriangleMeshShape*>(m_physics_object->GetShape());
                const btTriangleMesh* mesh = triangle_shape->m_mesh_ptr.get();

                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Shape] = _SerializeKey::Physics::Value::TriangleMesh;

                auto& vert_positions_json = json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::MeshVertexPositions];
                vert_positions_json = nlohmann::json::array();

                std::vector<glm::vec3> positions;
                positions.reserve(mesh->getNumTriangles() * 3);

                struct Callback : public btTriangleCallback
                {
                    std::vector<glm::vec3>& out;
                    Callback(std::vector<glm::vec3>& v) : out(v) {}

                    void processTriangle(btVector3* triangle, int, int) override 
                    {
                        out.emplace_back(triangle[0].x(), triangle[0].y(), triangle[0].z());
                        out.emplace_back(triangle[1].x(), triangle[1].y(), triangle[1].z());
                        out.emplace_back(triangle[2].x(), triangle[2].y(), triangle[2].z());
                    }
                };

                Callback cb(positions);

                triangle_shape->processAllTriangles(&cb, btVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX), btVector3(FLT_MAX, FLT_MAX, FLT_MAX));

                for (const glm::vec3& p : positions) 
                {
                    vert_positions_json.emplace_back(nlohmann::json::array({ p.x, p.y, p.z }));
                }
            }
            ///TODO: Huge hack, Improve this !!!!! 
            else if (shape_type == PhysicsShapeType::OWNED_COMPOUND_SHAPE)
            {
                const OwnedCompoundShape* compound = static_cast<OwnedCompoundShape*>(m_physics_object->GetShape());

                ENGINE_ASSERT(compound->GetFirstBaseShape()->getShapeType() == BOX_SHAPE_PROXYTYPE 
                              && "At SceneObject::SerializeToJson(): Serialization support for Owned Compouind Shape very lacking. Right now only one Box works.");

                const btBoxShape* shape = static_cast<const btBoxShape*>(compound->GetFirstBaseShape());
                const btVector3 half_extents = shape->getHalfExtentsWithoutMargin();
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::Shape]          = _SerializeKey::Physics::Value::Box;
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::BoxHalfExtents] = {{"x", half_extents.x()}, {"y", half_extents.y()}, {"z", half_extents.z()}};
            }
            else { ENGINE_ASSERT(false && "At SceneObject::SerializeToJson(): Invalid physics shape type."); }

            m_physics_object->GetShape()->setLocalScaling(scale); // Restore original scale
            
            //////////////////////////////////////////////// 
            //--------- Physics object type
            //////////////////////////////////////////////// 
            if (m_physics_object->GetType() == PhysicsObjectType::Ordinary)
            {
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::ObjectType] = _SerializeKey::Physics::Value::ObjectTypeOrdinary;
            }
            else if (m_physics_object->GetType() == PhysicsObjectType::Car)
            {
                json_obj[_SerializeKey::Physics::Root][_SerializeKey::Physics::ObjectType] = _SerializeKey::Physics::Value::ObjectTypeCar;
            }
            else { ENGINE_ASSERT(false && "At SceneObject::SerializeToJson(): Invalid physics object type."); }
        }

        return json_obj;
    }
}