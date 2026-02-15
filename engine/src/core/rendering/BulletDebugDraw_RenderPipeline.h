#pragma once

//Bullet
#include <btBulletDynamicsCommon.h>

//own
#include "core/rendering/DrawLines3D_RenderPipeline.h"
#include "core/rendering/DrawPoints3D_RenderPipeline.h"

namespace CoreEngine
{
    class BulletDebugDraw_RenderPipeline : public btIDebugDraw
    {
    public:
        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) noexcept override;

        virtual int getDebugMode() const noexcept override;
        virtual void setDebugMode(int debug_mode) noexcept override;
        virtual void reportErrorWarning(const char* warning_str) noexcept override;
        virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) noexcept override;
        virtual void draw3dText(const btVector3& location, const char* textString) noexcept override;

        void SetCameraMatrix(const glm::mat4& view_projection) noexcept;
        void SetCameraMatrixAndFrustumCull(const glm::mat4& view_projection) noexcept;
        void RenderAndClearData() noexcept;

    private:
        DrawLines3D_RenderPipeline  m_draw_lines_pipeline;
        DrawPoints3D_RenderPipeline m_draw_points_pipeline;

        int m_debug_mode = btIDebugDraw::DBG_DrawContactPoints | btIDebugDraw::DBG_DrawFrames;
    };

}