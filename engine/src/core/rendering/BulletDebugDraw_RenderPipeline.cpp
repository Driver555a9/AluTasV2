#include "core/rendering/BulletDebugDraw_RenderPipeline.h"

#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btVector3.h"
#include "Material.h"
#include "core/model/CapsuleModel.h"
#include "core/model/CylinderModel.h"
#include "core/model/Mesh.h"
#include "core/model/Model.h"
#include "core/rendering/Texture.h"
#include "core/scene/Scene3D_ObjectBuilder.h"
#include "core/scene/Scene3D_SceneObject.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/PhysicsUtility.h"
#include "core/model/BoxModel.h"
#include "core/model/SphereModel.h"
#include "core/model/PointsModel.h"

#include "core/utility/Assert.h"
#include "glm/ext/matrix_transform.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace CoreEngine
{
    BulletDebugDraw_RenderPipeline::BulletDebugDraw_RenderPipeline() noexcept 
    { 
        m_draw_lines_pipeline.SetLineThicknessFactor(10.0f);
        CoreEngine::Light light;
        light.m_position   = {0.0f, 0.0f, 0.0f};
        light.m_color      = {1.0f, 1.0f, 1.0f};
        light.m_intensity  = 100.0f;
        light.m_light_mode = CoreEngine::Light::LIGHT_MODE::DIRECT_LIGHT;
        m_primary_scene.EmplaceLightSource(light);
        m_static_scene.EmplaceLightSource(light);

        light.m_position  = {0, 0, 500};
        light.m_intensity = 200.0f;
        m_primary_scene.EmplaceLightSource(light);
        m_static_scene.EmplaceLightSource(light);
    }

    void BulletDebugDraw_RenderPipeline::SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& position) noexcept
    {
        m_draw_lines_pipeline.SetCameraData(cam_matrix);
        m_draw_points_pipeline.SetCameraMatrix(cam_matrix);
        m_indirect_3d_pipeline.SetCameraData(cam_matrix, position);
        m_static_indirect_pipeline.SetCameraData(cam_matrix, position);
    }
    
    void BulletDebugDraw_RenderPipeline::ClearAllData() noexcept
    {
        m_static_indirect_pipeline.SetSceneData({}, m_primary_scene.GetLightVectorConstRef());
        m_static_scene.ClearAllSceneObjects();
        m_static_mesh_instances.clear();
        ClearNonStaticData();
    }

    void BulletDebugDraw_RenderPipeline::ClearNonStaticData() noexcept
    {        
        m_draw_lines_pipeline.ClearAllLines();
        m_draw_points_pipeline.ClearAllPoints();
        m_indirect_3d_pipeline.SetSceneData({}, m_primary_scene.GetLightVectorConstRef());
        m_primary_scene.ClearAllSceneObjects();
        m_triangle_mesh_data_in_flight.clear();
    }

    void BulletDebugDraw_RenderPipeline::AddCompletedStaticMeshes(const std::vector<CompletedStaticMesh>& meshes) noexcept
    {
        for (const CompletedStaticMesh& m : meshes)
        {
            std::vector<Vertex> vertices;
            vertices.reserve(m.m_vertices.size());
            std::vector<GLuint> indices;
            indices.reserve(m.m_vertices.size());
            for (size_t i = 0; i < m.m_vertices.size(); ++i)
            {
                vertices.push_back(m.m_vertices[i]);
                indices.push_back(static_cast<GLuint>(i));
            }

            MaterialPBR material{ .m_base_color_factor = m.m_color };
            std::vector<Mesh> mesh_vec { Mesh{std::move(vertices), std::move(indices), std::make_shared<MaterialPBR>(material)} };

            Scene3D_ObjectBuilder builder = m_static_scene.CreateObjectBuilder();
            const glm::vec3 pos {0, 0, 0};
            const glm::quat rot = glm::identity<glm::quat>();
            auto model = std::make_unique<PointsModel>(std::move(mesh_vec), pos, rot, m.m_color);
            builder.RenderModel_SetExisting(std::move(model));
            Scene3D::ObjectID obj_id = m_static_scene.AddObjectFromBuilder(std::move(builder));

            m_static_mesh_instances[m.m_mesh_id] = obj_id;
        }
    }

    void BulletDebugDraw_RenderPipeline::UpdateStaticMeshInstance(uint64_t mesh_id, const btTransform& transform, const glm::vec3& scale) noexcept
    {
        auto it = m_static_mesh_instances.find(mesh_id);
        if (it == m_static_mesh_instances.end()) return;

        Scene3D_SceneObject* obj = m_static_scene.GetSceneObject(it->second);
        if (obj)
        {
            const auto pos = PUtil::ToGlm(transform.getOrigin());
            const auto rot = PUtil::ToGlm(transform.getRotation());
            obj->SetPosition(pos);
            obj->SetRotation(rot);
            obj->SetScale(scale);
        }
    }

    void BulletDebugDraw_RenderPipeline::Render() noexcept
    {
        if (! m_triangle_mesh_data_in_flight.empty())
        {
            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            const glm::vec3 pos {0};
            const glm::quat rot = glm::identity<glm::quat>();
            const glm::vec3 pseudo_color {1.0f};
            builder.RenderModel_SetExisting(std::make_unique<PointsModel>(std::move(m_triangle_mesh_data_in_flight), pos, rot, pseudo_color));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
            m_triangle_mesh_data_in_flight.clear();
        }

        //// Update dynamic scene
        if (m_primary_scene.GetAndResetObjectVecChangeFlag())
        {
            m_indirect_3d_pipeline.SetSceneData(m_primary_scene.GetRenderModelVector(), m_primary_scene.GetLightVectorConstRef());
        }
        else 
        {
            m_indirect_3d_pipeline.UpdateModelTransforms(m_primary_scene.GetRenderModelVector(), m_indirect_3d_pipeline.GetCameraRenderData().m_cam_matrix);
        }

        //// Update static scene
        if (m_static_scene.GetAndResetObjectVecChangeFlag())
        {
            m_static_indirect_pipeline.SetSceneData(m_static_scene.GetRenderModelVector(), m_static_scene.GetLightVectorConstRef());
        }
        else 
        {
            m_static_indirect_pipeline.UpdateModelTransforms(m_static_scene.GetRenderModelVector(), m_static_indirect_pipeline.GetCameraRenderData().m_cam_matrix);
        }

        m_static_indirect_pipeline.GetRenderConfigRef() = m_render_config.m_indirect_render_config;
        m_static_indirect_pipeline.Render();

        m_indirect_3d_pipeline.GetRenderConfigRef() = m_render_config.m_indirect_render_config;
        m_indirect_3d_pipeline.Render();

        m_render_config.m_use_depth_test_for_non_solid ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_draw_points_pipeline.Render();
        m_draw_lines_pipeline.Render();
        glEnable(GL_DEPTH_TEST);
    }

//////////////////////////////////////////////
// Drawing implementations
//////////////////////////////////////////////
    void BulletDebugDraw_RenderPipeline::DrawLineNonVirt(const btVector3& from, const btVector3& to, const btVector3& color) noexcept
    {
        m_draw_lines_pipeline.EmplaceBackLine(PUtil::ToGlm(from), PUtil::ToGlm(color));
        m_draw_lines_pipeline.EmplaceBackLine(PUtil::ToGlm(to), PUtil::ToGlm(color));
    }

    void BulletDebugDraw_RenderPipeline::DrawLineNonVirt(const btVector3& from, const btVector3& to, const btVector3& from_color, const btVector3& to_color) noexcept
    {
        m_draw_lines_pipeline.EmplaceBackLine(PUtil::ToGlm(from), PUtil::ToGlm(from_color));
        m_draw_lines_pipeline.EmplaceBackLine(PUtil::ToGlm(to), PUtil::ToGlm(to_color));
    }

    void BulletDebugDraw_RenderPipeline::DrawSphereNonVirt(float radius, const btTransform& trans, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_sphere_solid)
        {
            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<SphereModel>(radius, PUtil::ToGlm(trans.getOrigin()), PUtil::ToGlm(trans.getRotation()), PUtil::ToGlm(color)));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else 
        {
            const btVector3 center = trans.getOrigin();
            const btVector3 up   = trans.getBasis().getColumn(1);
            const btVector3 axis = trans.getBasis().getColumn(0);
            const btScalar minTh = -SIMD_HALF_PI;
            const btScalar maxTh = SIMD_HALF_PI;
            const btScalar minPs = -SIMD_HALF_PI;
            const btScalar maxPs = SIMD_HALF_PI;
            const btScalar stepDegrees = 30.f;
            DrawSpherePatchNonVirt(center, up, axis, radius, minTh, maxTh, minPs, maxPs, color, stepDegrees, false);
            DrawSpherePatchNonVirt(center, up, -axis, radius, minTh, maxTh, minPs, maxPs, color, stepDegrees, false);
        }
    }

    void BulletDebugDraw_RenderPipeline::DrawSphereNonVirt(const btVector3& p, float radius, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_sphere_solid)
        {
            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<SphereModel>(radius, PUtil::ToGlm(p), glm::identity<glm::quat>(), PUtil::ToGlm(color)));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else 
        {
            btTransform tr;
            tr.setIdentity();
            tr.setOrigin(p);
            DrawSphereNonVirt(radius, tr, color);
        }
    }

    void BulletDebugDraw_RenderPipeline::DrawTriangleNonVirt(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& n0, const btVector3& n1, const btVector3& n2, const btVector3& color, float alpha) noexcept
    {
        std::vector<Vertex> vertices;
        vertices.resize(3);
        vertices[0] = Vertex { PUtil::ToGlm(v0), PUtil::ToGlm(n0), {0, 0}};
        vertices[1] = Vertex { PUtil::ToGlm(v1), PUtil::ToGlm(n1), {0, 1}}; // Fixed index
        vertices[2] = Vertex { PUtil::ToGlm(v2), PUtil::ToGlm(n2), {1, 0}}; // Fixed index

        std::vector<GLuint> indices;
        indices.resize(3);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;

        MaterialPBR material {.m_base_color_factor = PUtil::ToGlm(color) };
        m_triangle_mesh_data_in_flight.emplace_back(std::move(vertices), std::move(indices), std::make_shared<MaterialPBR>(material));
    }   

    void BulletDebugDraw_RenderPipeline::DrawTriangleNonVirt(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, float alpha) noexcept
    {
        const glm::vec3 p0 = PUtil::ToGlm(v0);
        const glm::vec3 p1 = PUtil::ToGlm(v1);
        const glm::vec3 p2 = PUtil::ToGlm(v2);

        const glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        std::vector<Vertex> vertices;
        vertices.resize(3);
        vertices[0] = Vertex { PUtil::ToGlm(v0), normal, {0, 0}};
        vertices[1] = Vertex { PUtil::ToGlm(v1), normal, {0, 1}}; // Fixed index
        vertices[2] = Vertex { PUtil::ToGlm(v2), normal, {1, 0}}; // Fixed index

        std::vector<GLuint> indices;
        indices.resize(3);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;

        MaterialPBR material {.m_base_color_factor = PUtil::ToGlm(color) };
        m_triangle_mesh_data_in_flight.emplace_back(std::move(vertices), std::move(indices), std::make_shared<MaterialPBR>(material));
    }

    void BulletDebugDraw_RenderPipeline::DrawContactPointNonVirt(const btVector3& PointOnB, const btVector3& normalOnB, float distance, int lifeTime, const btVector3& color) noexcept
    {
        m_draw_points_pipeline.EmplaceBackPoint(PUtil::ToGlm(PointOnB), PUtil::ToGlm(color));
    }

    void BulletDebugDraw_RenderPipeline::DrawAabbNonVirt(const btVector3& from, const btVector3& to, const btVector3& color) noexcept
    {
        const btVector3 halfExtents = (to - from) * 0.5f;
		const btVector3 center = (to + from) * 0.5f;
		int i, j;

		btVector3 edgecoord(1.f, 1.f, 1.f), pa, pb;
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 3; j++)
			{
				pa = btVector3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
							   edgecoord[2] * halfExtents[2]);
				pa += center;

				int othercoord = j % 3;
				edgecoord[othercoord] *= -1.f;
				pb = btVector3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
							   edgecoord[2] * halfExtents[2]);
				pb += center;

				DrawLineNonVirt(pa, pb, color);
			}
			edgecoord = btVector3(-1.f, -1.f, -1.f);
			if (i < 3)
				edgecoord[i] *= -1.f;
		}
    }

    void BulletDebugDraw_RenderPipeline::DrawBoxNonVirt(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_box_solid)
        {
            const btVector3 bt_half_extents = (bbMax - bbMin) * btScalar(0.5);
            const btVector3 bt_center       = (bbMax + bbMin) * btScalar(0.5);

            const glm::vec3 half_extents = PUtil::ToGlm(bt_half_extents);
            const glm::vec3 center      = PUtil::ToGlm(bt_center);
            const glm::vec3 boxColor    = PUtil::ToGlm(color);

            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<BoxModel>(half_extents, center, glm::identity<glm::quat>(), boxColor));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else
        {
            DrawLineNonVirt(btVector3(bbMin[0], bbMin[1], bbMin[2]), btVector3(bbMax[0], bbMin[1], bbMin[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMin[1], bbMin[2]), btVector3(bbMax[0], bbMax[1], bbMin[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMax[1], bbMin[2]), btVector3(bbMin[0], bbMax[1], bbMin[2]), color);
		    DrawLineNonVirt(btVector3(bbMin[0], bbMax[1], bbMin[2]), btVector3(bbMin[0], bbMin[1], bbMin[2]), color);
		    DrawLineNonVirt(btVector3(bbMin[0], bbMin[1], bbMin[2]), btVector3(bbMin[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMin[1], bbMin[2]), btVector3(bbMax[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMax[1], bbMin[2]), btVector3(bbMax[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMin[0], bbMax[1], bbMin[2]), btVector3(bbMin[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMin[0], bbMin[1], bbMax[2]), btVector3(bbMax[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMin[1], bbMax[2]), btVector3(bbMax[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMax[0], bbMax[1], bbMax[2]), btVector3(bbMin[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(btVector3(bbMin[0], bbMax[1], bbMax[2]), btVector3(bbMin[0], bbMin[1], bbMax[2]), color);
        }
    }

    void BulletDebugDraw_RenderPipeline::DrawBoxNonVirt(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_box_solid)
        {
            const btVector3 bt_half_extents = (bbMax - bbMin) * btScalar(0.5);

            const glm::vec3 half_extents = PUtil::ToGlm(bt_half_extents);
            const glm::vec3 position     = PUtil::ToGlm(trans.getOrigin());
            const glm::quat rotation     = PUtil::ToGlm(trans.getRotation());
            const glm::vec3 box_color    = PUtil::ToGlm(color);

            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<BoxModel>(half_extents, position, rotation, box_color));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else
        {
            DrawLineNonVirt(trans * btVector3(bbMin[0], bbMin[1], bbMin[2]), trans * btVector3(bbMax[0], bbMin[1], bbMin[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMin[1], bbMin[2]), trans * btVector3(bbMax[0], bbMax[1], bbMin[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMax[1], bbMin[2]), trans * btVector3(bbMin[0], bbMax[1], bbMin[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMin[0], bbMax[1], bbMin[2]), trans * btVector3(bbMin[0], bbMin[1], bbMin[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMin[0], bbMin[1], bbMin[2]), trans * btVector3(bbMin[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMin[1], bbMin[2]), trans * btVector3(bbMax[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMax[1], bbMin[2]), trans * btVector3(bbMax[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMin[0], bbMax[1], bbMin[2]), trans * btVector3(bbMin[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMin[0], bbMin[1], bbMax[2]), trans * btVector3(bbMax[0], bbMin[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMin[1], bbMax[2]), trans * btVector3(bbMax[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMax[0], bbMax[1], bbMax[2]), trans * btVector3(bbMin[0], bbMax[1], bbMax[2]), color);
		    DrawLineNonVirt(trans * btVector3(bbMin[0], bbMax[1], bbMax[2]), trans * btVector3(bbMin[0], bbMin[1], bbMax[2]), color);
        }
    }

    void BulletDebugDraw_RenderPipeline::DrawCapsuleNonVirt(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_capsule_solid)
        {
            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<CapsuleModel>(radius, half_height * 2, up_axis, PUtil::ToGlm(transform.getOrigin()), PUtil::ToGlm(transform.getRotation()), PUtil::ToGlm(color)));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else
        {
            btIDebugDraw::drawCapsule(radius, half_height, up_axis, transform, color);
        }
    }

    void BulletDebugDraw_RenderPipeline::DrawCylinderNonVirt(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) noexcept
    {
        if (m_render_config.m_draw_cylinder_solid)
        {
            Scene3D_ObjectBuilder builder = m_primary_scene.CreateObjectBuilder();
            builder.RenderModel_SetExisting(std::make_unique<CylinderModel>(radius, half_height * 2, up_axis, PUtil::ToGlm(transform.getOrigin()), PUtil::ToGlm(transform.getRotation()), PUtil::ToGlm(color)));
            m_primary_scene.AddObjectFromBuilder(std::move(builder));
        }
        else
        {
            btIDebugDraw::drawCylinder(radius, half_height, up_axis, transform, color);
        }
    }

}