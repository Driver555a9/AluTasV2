#pragma once

//Bullet
#include <btBulletDynamicsCommon.h>

//own
#include "LinearMath/btIDebugDraw.h"
#include "core/model/Mesh.h"
#include "core/model/PointsModel.h"
#include "core/rendering/DrawLines3D_RenderPipeline.h"
#include "core/rendering/DrawPoints3D_RenderPipeline.h"
#include "core/rendering/IndirectDraw3D_RenderPipeline.h"
#include "core/scene/Scene3D.h"
#include "core/utility/Assert.h"

#include <unordered_map>

namespace CoreEngine
{
    class BulletDebugDraw_RenderPipeline : public btIDebugDraw
    {
    public:
        struct CompletedStaticMesh
        {
            uint64_t m_mesh_id;
            glm::vec3 m_color;
            std::vector<Vertex> m_vertices;
        };

        struct RenderConfig
        {
            RenderConfig() noexcept
            {
                m_indirect_render_config.m_use_cull_face = false;
                m_indirect_render_config.m_frustum_cull  = false;
            };

            int  m_debug_mode = btIDebugDraw::DBG_DrawContactPoints | btIDebugDraw::DBG_DrawFrames;
            bool m_use_depth_test_for_non_solid = true;
            bool m_draw_sphere_solid   = false;
            bool m_draw_box_solid      = false;
            bool m_draw_capsule_solid  = false;
            bool m_draw_cylinder_solid = false;
            IndirectDraw3D_RenderPipeline::RenderConfig m_indirect_render_config {};
        };
        BulletDebugDraw_RenderPipeline() noexcept;

        void SetRenderconfig(RenderConfig config) noexcept { m_render_config = config; }
        [[nodiscard]] RenderConfig GetRenderConfig() const noexcept { return m_render_config; }
        void SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& position) noexcept;
        void AddCompletedStaticMeshes(const std::vector<CompletedStaticMesh>& meshes) noexcept;
        void UpdateStaticMeshInstance(uint64_t mesh_id, const btTransform& transform, const glm::vec3& scale) noexcept;
        void ClearAllData() noexcept;
        void ClearNonStaticData() noexcept;
        void Render() noexcept;

        void DrawLineNonVirt(const btVector3& from, const btVector3& to, const btVector3& color) noexcept;
        void DrawLineNonVirt(const btVector3& from, const btVector3& to, const btVector3& from_color, const btVector3& to_color) noexcept;
        void DrawSphereNonVirt(float radius, const btTransform& trans, const btVector3& color) noexcept;
        void DrawSphereNonVirt(const btVector3& p, float radius, const btVector3& color) noexcept;
        void DrawTriangleNonVirt(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& n0, const btVector3& n1, const btVector3& n2, const btVector3& color, float alpha) noexcept;
        void DrawTriangleNonVirt(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, float alpha) noexcept;
        void DrawContactPointNonVirt(const btVector3& PointOnB, const btVector3& normalOnB, float distance, int lifeTime, const btVector3& color) noexcept;
        void DrawAabbNonVirt(const btVector3& from, const btVector3& to, const btVector3& color) noexcept;
        void DrawTransformNonVirt(const btTransform& transform, float orthoLen) noexcept 
        { 
            btIDebugDraw::drawTransform(transform, orthoLen); 
        }
        void DrawArcNonVirt(const btVector3& center, const btVector3& normal, const btVector3& axis, float radiusA, float radiusB, float minAngle, float maxAngle, const btVector3& color, bool drawSect, float stepDegrees = float(10.f)) noexcept 
        {
            btIDebugDraw::drawArc(center, normal, axis, radiusA, radiusB, minAngle, maxAngle, color, drawSect, stepDegrees); 
        }
        void DrawSpherePatchNonVirt(const btVector3& center, const btVector3& up, const btVector3& axis, float radius, float minTh, float maxTh, float minPs, float maxPs, const btVector3& color, float stepDegrees = float(10.f), bool drawCenter = true) noexcept 
        { 
            btIDebugDraw::drawSpherePatch(center, up, axis, radius, minTh, maxTh, minPs, maxPs, color, stepDegrees, drawCenter); 
        }
        void DrawBoxNonVirt(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color) noexcept;
        void DrawBoxNonVirt(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color) noexcept;
        void DrawCapsuleNonVirt(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) noexcept;
        void DrawCylinderNonVirt(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) noexcept;
        void DrawConeNonVirt(float radius, float height, int upAxis, const btTransform& transform, const btVector3& color) noexcept 
        { 
            btIDebugDraw::drawCone(radius, height, upAxis, transform, color); 
        }
        void DrawPlaneNonVirt(const btVector3& plane_normal, float plane_const, const btTransform& transform, const btVector3& color) noexcept 
        { 
            btIDebugDraw::drawPlane(plane_normal, plane_const, transform, color); 
        }

        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override 
        { 
            DrawLineNonVirt(from, to, color); 
        }
        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& from_color, const btVector3& to_color) override 
        { 
            DrawLineNonVirt(from, to, from_color, to_color); 
        }
        virtual void drawSphere(float radius, const btTransform& trans, const btVector3& color) override 
        { 
            DrawSphereNonVirt(radius, trans, color); 
        }
        virtual void drawSphere(const btVector3& p, float radius, const btVector3& color) override 
        { 
            DrawSphereNonVirt(p, radius, color); 
        }
        virtual void drawTriangle(const btVector3& v0, const btVector3& v1, const btVector3& v2, 
                                  const btVector3& n0, const btVector3& n1, const btVector3& n2, const btVector3& color, float alpha) override 
        {
            DrawTriangleNonVirt(v0, v1, v2, n0, n1, n2, color, alpha);
        }
        virtual void drawTriangle(const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, float alpha) override 
        { 
            DrawTriangleNonVirt(v0, v1, v2, color, alpha);
        }
        virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, float distance, int lifeTime, const btVector3& color) override 
        { 
            DrawContactPointNonVirt(PointOnB, normalOnB, distance, lifeTime, color);
        }
        virtual void reportErrorWarning(const char* warningString) noexcept override 
        { 
            ENGINE_ERROR_PRINT("Bullet Error: " << warningString); 
        }
        virtual void draw3dText(const btVector3& location, const char* textString) noexcept override {}
        virtual void setDebugMode(int debug_mode) noexcept override 
        {
            m_render_config.m_debug_mode = debug_mode;
        }
        virtual int  getDebugMode() const noexcept override 
        { 
            return m_render_config.m_debug_mode; 
        }
        virtual void drawAabb(const btVector3& from, const btVector3& to, const btVector3& color) noexcept override 
        { 
            DrawAabbNonVirt(from, to, color); 
        }
        virtual void drawTransform(const btTransform& transform, float orthoLen) noexcept override 
        { 
            btIDebugDraw::drawTransform(transform, orthoLen); 
        }
        virtual void drawArc(const btVector3& center, const btVector3& normal, const btVector3& axis, float radiusA, float radiusB, float minAngle, float maxAngle,
                            const btVector3& color, bool drawSect, float stepDegrees = float(10.f)) noexcept override 
        {
            btIDebugDraw::drawArc(center, normal, axis, radiusA, radiusB, minAngle, maxAngle, color, drawSect, stepDegrees); 
        }
        virtual void drawSpherePatch(const btVector3& center, const btVector3& up, const btVector3& axis, float radius, float minTh, float maxTh, float minPs, float maxPs, const btVector3& color, 
                                    float stepDegrees = float(10.f), bool drawCenter = true) noexcept override 
        { 
            btIDebugDraw::drawSpherePatch(center, up, axis, radius, minTh, maxTh, minPs, maxPs, color, stepDegrees, drawCenter); 
        }
        virtual void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color) override 
        { 
            DrawBoxNonVirt(bbMin, bbMax, color);  
        }
        virtual void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color) override 
        { 
            DrawBoxNonVirt(bbMin, bbMax, trans, color);
        }
        virtual void drawCapsule(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) override 
        { 
            DrawCapsuleNonVirt(radius, half_height, up_axis, transform, color); 
        }
        virtual void drawCylinder(float radius, float half_height, int up_axis, const btTransform& transform, const btVector3& color) override 
        { 
            DrawCylinderNonVirt(radius, half_height, up_axis, transform, color); 
        }
        virtual void drawCone(float radius, float height, int upAxis, const btTransform& transform, const btVector3& color) noexcept override 
        { 
            btIDebugDraw::drawCone(radius, height, upAxis, transform, color); 
        }
        virtual void drawPlane(const btVector3& plane_normal, float plane_const, const btTransform& transform, const btVector3& color) noexcept override 
        { 
            btIDebugDraw::drawPlane(plane_normal, plane_const, transform, color); 
        }
        virtual void clearLines() noexcept override {}
        virtual void flushLines() noexcept override {}

    private:
        std::unordered_map<uint64_t, Scene3D::ObjectID> m_static_mesh_instances;
        std::vector<Mesh> m_triangle_mesh_data_in_flight;
        DrawLines3D_RenderPipeline  m_draw_lines_pipeline;
        DrawPoints3D_RenderPipeline m_draw_points_pipeline;
        IndirectDraw3D_RenderPipeline m_indirect_3d_pipeline;
        Scene3D m_primary_scene;
        RenderConfig m_render_config;

        IndirectDraw3D_RenderPipeline m_static_indirect_pipeline;
        Scene3D m_static_scene;
    };

}