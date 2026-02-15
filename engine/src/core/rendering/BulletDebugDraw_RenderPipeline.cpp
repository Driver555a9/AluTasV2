#include "core/rendering/BulletDebugDraw_RenderPipeline.h"

#include "core/utility/PhysicsUtility.h"

#include "core/utility/Assert.h"

namespace CoreEngine
{
    void BulletDebugDraw_RenderPipeline::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) noexcept
    {
        m_draw_lines_pipeline.EmplaceBackLine(PhysicsUtility::BtVector3ToGlm(from), PhysicsUtility::BtVector3ToGlm(color));
        m_draw_lines_pipeline.EmplaceBackLine(PhysicsUtility::BtVector3ToGlm(to), PhysicsUtility::BtVector3ToGlm(color));
    }

    int BulletDebugDraw_RenderPipeline::getDebugMode() const noexcept
    {
        return m_debug_mode;
    }

    void BulletDebugDraw_RenderPipeline::setDebugMode(int debug_mode) noexcept
    {
        m_debug_mode = debug_mode;
    }

    void BulletDebugDraw_RenderPipeline::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) noexcept
    {
        m_draw_points_pipeline.EmplaceBackPoint(PhysicsUtility::BtVector3ToGlm(PointOnB), PhysicsUtility::BtVector3ToGlm(color));
    }

    void BulletDebugDraw_RenderPipeline::SetCameraMatrix(const glm::mat4& cam_matrix) noexcept
    {
        m_draw_lines_pipeline.SetCameraMatrix(cam_matrix);
        m_draw_points_pipeline.SetCameraMatrix(cam_matrix);
    }

    void BulletDebugDraw_RenderPipeline::SetCameraMatrixAndFrustumCull(const glm::mat4& view_projection) noexcept
    {
        m_draw_lines_pipeline.SetCameraMatrixAndFrustumCull(view_projection);
        m_draw_points_pipeline.SetCameraMatrix(view_projection);
    }

    void BulletDebugDraw_RenderPipeline::RenderAndClearData() noexcept
    {
        glDisable(GL_DEPTH_TEST);
        //////////////////////////////////////////////// 
        //--------- Draw lines
        ////////////////////////////////////////////////
        m_draw_lines_pipeline.Render();
        m_draw_lines_pipeline.ClearAllLines();

        //////////////////////////////////////////////// 
        //--------- Draw Points
        ////////////////////////////////////////////////
        m_draw_points_pipeline.Render();
        m_draw_points_pipeline.ClearAllPoints();
        
        glEnable(GL_DEPTH_TEST);
    }
    
    //////////////////////////////////////////////// 
    //---------  Not implemented
    ////////////////////////////////////////////////
    void BulletDebugDraw_RenderPipeline::reportErrorWarning(const char* warning_str) noexcept {}
    void BulletDebugDraw_RenderPipeline::draw3dText(const btVector3& location, const char* textString) noexcept{}

}