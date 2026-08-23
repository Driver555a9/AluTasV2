#include "layer/DebugDrawStreamLayer.h"
#include "BulletDebugDrawStream.h"
#include "BulletTypes.h"
#include "Communication.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "common/CameraState.h"
#include "common/RacerState.h"
#include "core/utility/Assert.h"
#include "core/application/Application.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/PhysicsUtility.h"
#include "core/utility/Timer.h"
#include "globalstate/AsphaltDllManager.h"

#include "core/event/EventDispatcher.h"
#include "core/event/WindowEvents.h"
#include "core/event/InputEvents.h"
#include "core/event/ApplicationStateEvents.h"

#include "imgui.h"

#include <cstdint>
#include <memory>

namespace AsphaltTas
{
    DebugDrawStreamLayer::DebugDrawStreamLayer(CoreEngine::Window::Handle handle) noexcept 
    : CoreEngine::Basic_Layer(handle), m_camera(glm::vec3(0.0f), CoreEngine::Application::Get()->GetWindowPtr(m_handle)->GetAspectRatio(), 50, 0.1f)
    {
        s_instance = this;
        AsphaltDllManager::GetDllGeneralCommandsInRef()->m_write_meta_data.m_update_debug_draw_stream = true;
    }
    
    DebugDrawStreamLayer::~DebugDrawStreamLayer() noexcept
    {
        s_instance = nullptr;
        AsphaltDllManager::GetDllGeneralCommandsInRef()->m_write_meta_data.m_update_debug_draw_stream = false;
    }   

    void DebugDrawStreamLayer::OnEvent(CoreEngine::Basic_Event& e) noexcept 
    {
        CoreEngine::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<CoreEngine::FramebufferResizeEvent>( 
        [this](CoreEngine::FramebufferResizeEvent& e) -> bool 
        { 
            m_camera.SetAspectRatio(static_cast<float>(e.GetWidth()) / e.GetHeight());
            return false;
        });
    }
    
    void DebugDrawStreamLayer::OnUpdate(CoreEngine::Units::MicroSecond dt) noexcept 
    {
        
    }
    
    void DebugDrawStreamLayer::OnRender() noexcept 
    {
        m_has_signal = false;
        {
            ScopeLockedAccess<std::unique_ptr<BulletTypes::DebugDrawStream::DebugFrameData>> debug_state_opt_ref = AsphaltDllManager::GetDllOutDebugFrameDataResultRef();
            if (debug_state_opt_ref.Get() != nullptr)
            {
                m_has_signal = true;
                uint32_t command_count = debug_state_opt_ref.Get()->m_command_count;
                BulletTypes::DebugDrawStream::DrawCommand* commands = debug_state_opt_ref.Get()->m_commands;

                const auto ToBtVec3NoConvert = [](const BulletTypes::Vector3& vec)
                {
                    return btVector3 {vec.x, vec.y, vec.z};
                };

                const auto ToBtVec3ConvXYZ = [](const BulletTypes::Vector3& vec)
                {
                    return btVector3 {vec.x, vec.z, -vec.y};
                };

                const auto ToBtTransConvXYZ = [](const BulletTypes::Transform& trans)
                {
                    ComDllOut::RecordedRacerState raw{};
                    raw.m_racer_transform_mat4x4 = std::bit_cast<BulletTypes::UnalignedTransform>(trans);
                    AsphaltTas::RacerState state(raw);

                    const glm::vec3 pos  = state.GetPositionOpenGL_XYZ();
                    const glm::quat rot  = state.GetRotationOpenGL_XYZ();

                    btTransform out;
                    out.setOrigin(btVector3(pos.x, pos.y, pos.z));
                    out.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
                    return out;
                };

                for (uint32_t i = 0; i < command_count; ++i)
                {
                    BulletTypes::DebugDrawStream::DrawCommand& cmd = commands[i];
                    using CmdType = BulletTypes::DebugDrawStream::DrawCmdType;

                    switch (cmd.m_type)
                    {
                        case CmdType::None:
                            break;

                        case CmdType::Line:
                        {
                            const auto& line = cmd.m_data.m_line;
                            m_bullet_debug_pipeline.DrawLineNonVirt(ToBtVec3ConvXYZ(line.m_from), ToBtVec3ConvXYZ(line.m_to), ToBtVec3NoConvert(line.m_from_color));
                            break;            
                        }

                        case CmdType::Sphere:
                        {
                            const auto& sphere = cmd.m_data.m_sphere;
                            const float radius = sphere.m_radius * 0.5f; // TODO: FIX THIS, WHY HALF NEEDED
                            if (sphere.m_has_transform)
                            {
                                m_bullet_debug_pipeline.DrawSphereNonVirt(radius, ToBtTransConvXYZ(sphere.m_transform), ToBtVec3NoConvert(sphere.m_color));
                            }
                            else 
                            {
                                m_bullet_debug_pipeline.DrawSphereNonVirt(ToBtVec3ConvXYZ(sphere.m_center), radius, ToBtVec3NoConvert(sphere.m_color));
                            }
                            break;
                        }

                        case CmdType::Triangle:
                        {
                            const auto& tri = cmd.m_data.m_triangle;
                            if (tri.m_has_normals)
                            {
                                m_bullet_debug_pipeline.DrawTriangleNonVirt(ToBtVec3ConvXYZ(tri.m_v0), ToBtVec3ConvXYZ(tri.m_v1),ToBtVec3ConvXYZ(tri.m_v2), 
                                        ToBtVec3ConvXYZ(tri.m_n0), ToBtVec3ConvXYZ(tri.m_n1), ToBtVec3ConvXYZ(tri.m_n2), ToBtVec3NoConvert(tri.m_color), tri.m_alpha);
                            }
                            else
                            {
                                m_bullet_debug_pipeline.DrawTriangleNonVirt(ToBtVec3ConvXYZ(tri.m_v0), ToBtVec3ConvXYZ(tri.m_v1), ToBtVec3ConvXYZ(tri.m_v2),
                                                                    ToBtVec3NoConvert(tri.m_color), tri.m_alpha);
                            }
                            break;
                        }

                        case CmdType::ContactPoint:
                        {
                            const auto& cp = cmd.m_data.m_contact_point;
                            m_bullet_debug_pipeline.DrawContactPointNonVirt(ToBtVec3ConvXYZ(cp.m_point_on_B), ToBtVec3ConvXYZ(cp.m_normal_on_B), cp.m_distance, cp.m_life_time, 
                                                                    ToBtVec3NoConvert(cp.m_color));
                            break;
                        }

                        case CmdType::AABB:
                        {
                            const auto& aabb = cmd.m_data.m_aabb;
                            m_bullet_debug_pipeline.DrawAabbNonVirt(ToBtVec3ConvXYZ(aabb.m_from), ToBtVec3ConvXYZ(aabb.m_to), ToBtVec3NoConvert(aabb.m_color));
                            break;
                        }

                        case CmdType::Transform:
                        {
                            const auto& tf = cmd.m_data.m_transform;
                            m_bullet_debug_pipeline.DrawTransformNonVirt(ToBtTransConvXYZ(tf.m_transform), tf.m_ortho_len);
                            break;
                        }

                        case CmdType::Box:
                        {
                            const auto& box = cmd.m_data.m_box;
                            if (box.m_has_transform)
                            {
                                m_bullet_debug_pipeline.DrawBoxNonVirt(ToBtVec3ConvXYZ(box.m_bb_min), ToBtVec3ConvXYZ(box.m_bb_max), ToBtTransConvXYZ(box.m_transform), 
                                                                ToBtVec3NoConvert(box.m_color));
                            }
                            else
                            {
                                m_bullet_debug_pipeline.DrawBoxNonVirt(ToBtVec3ConvXYZ(box.m_bb_min), ToBtVec3ConvXYZ(box.m_bb_max), ToBtVec3NoConvert(box.m_color));
                            }
                            break;
                        }

                        case CmdType::Capsule:
                        {
                            const auto& cap = cmd.m_data.m_capsule;
                            m_bullet_debug_pipeline.DrawCapsuleNonVirt(cap.m_radius, cap.m_half_height, cap.m_up_axis, ToBtTransConvXYZ(cap.m_transform),
                                                                ToBtVec3NoConvert(cap.m_color));
                            break;
                        }

                        case CmdType::Cylinder:
                        {
                            const auto& cyl = cmd.m_data.m_cylinder;
                            m_bullet_debug_pipeline.DrawCylinderNonVirt(cyl.m_radius, cyl.m_half_height, cyl.m_up_axis, ToBtTransConvXYZ(cyl.m_transform), ToBtVec3NoConvert(cyl.m_color));
                            break;
                        }

                        case CmdType::Cone:
                        {
                            const auto& cone = cmd.m_data.m_cone;
                            m_bullet_debug_pipeline.DrawConeNonVirt(cone.m_radius, cone.m_height, cone.m_up_axis, ToBtTransConvXYZ(cone.m_transform), ToBtVec3NoConvert(cone.m_color));
                            break;
                        }

                        case CmdType::Plane:
                        {
                            const auto& plane = cmd.m_data.m_plane;
                            m_bullet_debug_pipeline.DrawPlaneNonVirt(ToBtVec3ConvXYZ(plane.m_plane_normal), plane.m_plane_const, ToBtTransConvXYZ(plane.m_transform), ToBtVec3NoConvert(plane.m_color));
                            break;
                        }

                        case CmdType::CachedMeshInstance:
                        {
                            const auto& cached = cmd.m_data.m_cached_instance;
                            const auto real_trans = ToBtTransConvXYZ(cached.m_transform);
                            
                            const btQuaternion yaw_flip(btVector3(0.0f, 1.0f, 0.0f), SIMD_PI);
                            const btQuaternion corrected_rot = real_trans.getRotation() * yaw_flip; //@TODO: FIX THIS HACK, WE SHOULD NOT HAVE TO FLIP ROTATION!

                            btTransform trans;
                            trans.setIdentity();
                            trans.setOrigin(real_trans.getOrigin());
                            trans.setRotation(corrected_rot);

                            m_bullet_debug_pipeline.UpdateStaticMeshInstance(cached.m_mesh_id, trans, { cached.m_scale.x, cached.m_scale.z, cached.m_scale.y });
                            break;
                        }

                        case CmdType::ResetAllDataPseudoCmd:
                        {
                            m_bullet_debug_pipeline.ClearAllData();
                            m_has_signal = false;
                            cmd.m_type = CmdType::None;
                            break;
                        }
                    }
                }
            }
        }

        const auto completed_static = AsphaltDllManager::TakeNewlyCompletedStaticMeshes();
        if (! completed_static.empty())
        {
            m_bullet_debug_pipeline.AddCompletedStaticMeshes(completed_static);
        }

        if (m_has_signal)
        {
            std::optional<ComDllOut::DllStateOut> copy = AsphaltDllManager::GetDllStateOutCopy();
            if (copy.has_value())
            {
                CameraState cam_state (copy->m_camera_state);

                m_camera.SetPosition(cam_state.GetPositionOpenGL_XYZ());
                m_camera.SetRotation(cam_state.GetRotationOpenGL_WXYZ());
                m_camera.SetFovRad(cam_state.GetFovRadians());
            }
            m_bullet_debug_pipeline.SetCameraData(m_camera.GetCameraMatrix(), m_camera.GetPosition());
            
            m_bullet_debug_pipeline.Render();
            m_bullet_debug_pipeline.ClearNonStaticData();
        }
    }
    
    void DebugDrawStreamLayer::OnImGuiRender() noexcept
    {
        if (!m_has_signal)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);

            constexpr ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse;

            ImGui::Begin("No Signal error", nullptr, flags);

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            constexpr const char* text = "No Signal";
            constexpr float paddingX = 24.0f;
            constexpr float paddingY = 16.0f;
            constexpr float rounding = 6.0f;

            const ImVec2 text_size = ImGui::CalcTextSize(text);
            const ImVec2 box_size{ text_size.x + paddingX * 2.0f, text_size.y + paddingY * 2.0f };
            const ImVec2 window_pos  = ImGui::GetWindowPos();
            const ImVec2 window_size = ImGui::GetWindowSize();
            const ImVec2 center{ window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f };
            const ImVec2 min { center.x - box_size.x * 0.5f, center.y - box_size.y * 0.5f };
            const ImVec2 max { center.x + box_size.x * 0.5f, center.y + box_size.y * 0.5f };
            drawList->AddRectFilled(min,  max, IM_COL32(180, 30, 30, 255), rounding);
            drawList->AddText({center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f}, IM_COL32(255, 255, 255, 255), text);

            ImGui::End();
        }
    }

    void DebugDrawStreamLayer::CreateInstance() noexcept
    {
        ENGINE_ASSERT( ! s_instance && "There should only ever be one DebugDrawStreamLayer active at one time.");

        using Cdis = CoreEngine::Window::WindowCreationConfig::CallbackDisableFlags;

        constexpr CoreEngine::Window::WindowCreationConfig config 
        {
            .m_title                       = "DebugDrawStream",
            .m_relative_size               = {1.0f, 0.95f},
            .m_callback_disable_flags      = {},
            .m_imgui_flags                 = {},
            .m_MSAA_sample_count           = 8,
            .m_is_decorated                = true,
            .m_has_transparent_framebuffer = false,
            .m_is_clickthrough             = false
        };

        CoreEngine::Application::Get()->QueueCreateWindowAndPushLayer<DebugDrawStreamLayer>(config);
    }

    bool DebugDrawStreamLayer::InstanceExists() noexcept
    {
        return s_instance != nullptr;
    }

    void DebugDrawStreamLayer::DeleteInstance() noexcept
    {
        if (! InstanceExists()) return;
        CoreEngine::Application::Get()->QueueDeleteWindowLayerStack(s_instance->m_handle);
    }
}