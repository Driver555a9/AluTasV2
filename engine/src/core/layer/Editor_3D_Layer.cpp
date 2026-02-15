#include "core/layer/Editor_3D_Layer.h"

//Own includes
#include "core/application/ApplicationGlobalState.h"

#include "core/utility/PhysicsUtility.h"
#include "core/utility/CommonUtility.h"
#include "core/utility/Performance.h"

#include "core/model/PointsModel.h"

//GLFW
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

//ImGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "imgui/ImGuiFileDialog.h"

//std
#include <array>
#include <filesystem>

namespace CoreEngine
{
//////////////////////////////////////////////// 
//---------  Internal Helpers
//////////////////////////////////////////////// 
    MathUtility::Ray3D Editor_3D_Layer::ScreenPointToRay(const double mouse_x, const double mouse_y) const noexcept
    {
        const auto [width, height] = GlobalGet<GlobalGet_FramebufferSize>();

        const float ndc_x = (2.0f * mouse_x) / width - 1.0f;
        const float ndc_y = 1.0f - (2.0f * mouse_y) / height;

        const glm::vec4 ray_clip(ndc_x, ndc_y, -1.0f, 1.0f);

        glm::vec4 ray_eye = glm::inverse(m_camera.CalculateProjectionMatrix()) * ray_clip;
        ray_eye.z = -1.0f;
        ray_eye.w = 0.0f;

        const glm::vec3 ray_world = glm::normalize(glm::vec3(glm::inverse(m_camera.CalculateViewMatrix()) * ray_eye));

        return MathUtility::Ray3D { m_camera.GetPosition(), ray_world, 100'000.0f };
    }

    void Editor_3D_Layer::OnSetSelectedSceneObject(Scene3D_SceneObject* obj) noexcept
    {
        // Change distance such that OrbitalCamera will always have appropriate distance
        if (m_selected_object_state.m_object_ptr != obj && obj && m_camera_controller->GetType() == Basic_CameraController::Type::OrbitalCam)
        {
            const float selected_obj_bounding_radius = std::max(0.1f, obj->GetWorldSpaceMaxBoundingSphereRadius() * 1.5f);
            static_cast<OrbitalCam_CameraController*>(m_camera_controller.get())->SetDistance(selected_obj_bounding_radius);
        }
        
        m_selected_object_state.m_object_ptr = obj;
    }

    void Editor_3D_Layer::OnSceneResetAndLoadFromPath(const std::string& path) noexcept
    {
        OnSetSelectedSceneObject(nullptr); //Insure no dangling pointers
        m_camera = m_scene.LoadFromSerializedFile(path);
    }
    
//////////////////////////////////////////////// 
//---------  Process Events
//////////////////////////////////////////////// 
    bool Editor_3D_Layer::OnMousePressed(MousePressedEvent& e) noexcept
    {
        if (ImGui::GetIO().WantCaptureMouse || e.GetMouseButton() != GLFW_MOUSE_BUTTON_LEFT) 
        { 
            return Freecam_3D_Layer::OnMousePressed(e); 
        }

        const MathUtility::Ray3D ray        = ScreenPointToRay(e.GetX(), e.GetY());
        const Scene3D::RaycastHit scene_hit = m_scene.RaycastSelect(ray);

        //////////////////////////////////////////////// 
        //---------  Select object that was clicked
        //////////////////////////////////////////////// 
        if (! m_next_left_click_places_triangle_point)
        {
            OnSetSelectedSceneObject(scene_hit.m_scene_object_ptr);
            return Freecam_3D_Layer::OnMousePressed(e);
        }

        //////////////////////////////////////////////// 
        //---------  Place point for Triangle object creation
        //////////////////////////////////////////////// 
        glm::vec3 point_pos;

        if (scene_hit.m_scene_object_ptr)
        {
            point_pos = scene_hit.m_intersection_point;
        }
        else 
        {
            const MathUtility::Plane plane { .m_point = {0.0f, 0.0f, 0.0f}, .m_normal_normalized = {0.0f, 1.0f, 0.0f} };
            const MathUtility::Raytest3D_Result plane_hit = MathUtility::RayPlaneIntersect(ray, plane);

            if (! plane_hit.m_has_hit)  // We have neither a hit in scene, nor did we hit the xz plane
            {
                return Freecam_3D_Layer::OnMousePressed(e);
            }

            point_pos = plane_hit.m_intersection_point;
        }

        //Placeholder values
        const glm::vec3 normal      {0.0f};
        const glm::vec2 texuv       {0.0f};
        m_triangle_point_creation_data.m_points.emplace_back(point_pos, normal, texuv);

        return Freecam_3D_Layer::OnMousePressed(e);
    }

    bool Editor_3D_Layer::OnMouseMoved(MouseMovedEvent& e) noexcept
    {
        if (   ImGui::GetIO().WantCaptureMouse
            //Do not move if not clicking left to drag it
            || !m_input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_LEFT]
            //Do nothing if no model selected
            || !m_selected_object_state.m_object_ptr
            // Do not allow dragging whilst in OrbitalCamera 
            || m_camera_controller->GetType() != Basic_CameraController::Type::FreeCam)
        {
            return Freecam_3D_Layer::OnMouseMoved(e);
        }

        const glm::vec3 obj_position = m_selected_object_state.m_object_ptr->GetPosition();

        const MathUtility::Plane plane { .m_point = obj_position, .m_normal_normalized = m_camera.GetForwardDirection() };
        const MathUtility::Raytest3D_Result intersection_point = MathUtility::RayPlaneIntersect(ScreenPointToRay(e.GetX(), e.GetY()), plane);
        
        if (! intersection_point.m_has_hit)
        {
            return Freecam_3D_Layer::OnMouseMoved(e);
        }

        m_selected_object_state.m_object_ptr->SetPosition(intersection_point.m_intersection_point);

        return Freecam_3D_Layer::OnMouseMoved(e);
    }

    bool Editor_3D_Layer::OnMouseScrolled(MouseScrolledEvent& e) noexcept
    {
        if (! m_input_state.m_mouse_is_pressed[GLFW_MOUSE_BUTTON_LEFT] || m_selected_object_state.m_object_ptr == nullptr || ImGui::GetIO().WantCaptureMouse)
            return Freecam_3D_Layer::OnMouseScrolled(e);

        Basic_Model*   render_model = m_selected_object_state.m_object_ptr->m_render_model.get();
        PhysicsObject* physics_obj  = m_selected_object_state.m_object_ptr->m_physics_object.get();

        if (!render_model && !physics_obj)
        {
            return Freecam_3D_Layer::OnMouseScrolled(e);
        }

        const glm::vec3 obj_position = m_selected_object_state.m_object_ptr->GetPosition();
        
        const glm::vec3 obj_to_camera_vec = m_camera.GetPosition() - obj_position;

        constexpr const float MOVEMENT_PER_TICK = 0.05f;

        const glm::vec3 moved_pos = obj_position + (obj_to_camera_vec * MOVEMENT_PER_TICK * static_cast<float>(e.GetYOffset()));

        m_selected_object_state.m_object_ptr->SetPosition(moved_pos);

        return false;
    }

//////////////////////////////////////////////// 
//---------  Basic_Layer abstract methods
//////////////////////////////////////////////// 
    void Editor_3D_Layer::OnUpdate(Units::MicroSecond delta_time)
    {
        //////////////////////////////////////////////// 
        //---------  If no longer selected object, quit orbital cam
        //////////////////////////////////////////////// 
        if (m_camera_controller->GetType() == Basic_CameraController::Type::OrbitalCam)
        {
            if (const auto* obj = m_selected_object_state.m_object_ptr)
                static_cast<OrbitalCam_CameraController*>(m_camera_controller.get())->SetTarget(obj->GetPosition());
            else 
                OnChangeCameraController<FreeCam_CameraController>();
        }
        else if (m_camera_controller->GetType() == Basic_CameraController::Type::FollowCam)
        {
            if (const auto* obj = m_selected_object_state.m_object_ptr)
                static_cast<FollowCam_CameraController*>(m_camera_controller.get())->SetTarget(obj->GetPosition(), obj->GetRotation());
            else 
                OnChangeCameraController<FreeCam_CameraController>();
        }

        //////////////////////////////////////////////// 
        //---------  Physics objects (e.g. Car) will be updated
        //////////////////////////////////////////////// 
        if (m_selected_object_state.m_object_ptr && m_selected_object_state.m_object_ptr->m_physics_object && m_selected_object_state.m_physics_receives_input)
        {
            m_selected_object_state.m_object_ptr->m_physics_object->OnInput(m_input_state, delta_time);
        }

        Freecam_3D_Layer::OnUpdate(delta_time);
    }

    void Editor_3D_Layer::OnEvent(Basic_Event& event)
    {
        Freecam_3D_Layer::OnEvent(event);
    }

    void Editor_3D_Layer::OnRender()
    {
        Freecam_3D_Layer::OnRender();
    
        const glm::mat4 cam_matrix = m_camera.CalculateCameraMatrix();

        if (m_selected_object_state.m_object_ptr && m_selected_object_state.m_highlight_aabb_box)
        {
            const glm::vec3 color (1.0f, 0.0f, 0.0f);
            m_draw_lines_pipeline.SetLineData(m_selected_object_state.m_object_ptr->CalculateAABBDebugLines(), color);
            m_draw_lines_pipeline.SetCameraMatrixAndFrustumCull(cam_matrix);
            m_draw_lines_pipeline.Render();
            m_draw_lines_pipeline.ClearAllLines();
        } 

        if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::TRIANGLE_POINTS))
        {
            m_draw_points_pipelines.SetCameraMatrix(cam_matrix);
            std::vector<glm::vec3> points;
            points.reserve(m_triangle_point_creation_data.m_points.size());
            for (const Vertex& point : m_triangle_point_creation_data.m_points)
            {
                points.push_back(point.m_position);
            }
            m_draw_points_pipelines.SetPoints(points, m_triangle_point_creation_data.m_color);
            m_draw_points_pipelines.Render();
            m_draw_points_pipelines.ClearAllPoints();
        }
    }

    void Editor_3D_Layer::OnImGuiRender()
    {
        OnImGuiRender_TopNavigationBar(); // order is relevant - they rely on the last updating relevant size variables
        OnImGuiRender_LeftOptionPanel();
        OnImGuiRender_BottomOptionPanel();
        OnImGuiRender_RightOptionPanel();
    }

//////////////////////////////////////////////// 
//---------  Creating objects
//////////////////////////////////////////////// 
    bool Editor_3D_Layer::OnCreateSphere(const SphereCreationData& data) noexcept
    {
        Scene3D_ObjectBuilder builder = m_scene.CreateObjectBuilder();

        builder.SetPosition(data.m_position);
        builder.SetRotation(glm::identity<glm::quat>());
        builder.SetName(data.m_name);

        builder.RenderAndCollision_SetSphere(data.m_radius, data.m_color);
        if (data.m_has_physics)
        {
            builder.SetPhysicsObjectType(CoreEngine::PhysicsObjectType::Ordinary);
            builder.SetMass(std::max<Units::Kilogram>(data.m_mass, Units::Kilogram(0.0f)));
        }
        m_scene.AddObjectFromBuilder(std::move(builder));

        return true;
    }

    bool Editor_3D_Layer::OnCreateBox(const BoxCreationData& data) noexcept
    {
        Scene3D_ObjectBuilder builder = m_scene.CreateObjectBuilder();

        builder.SetPosition(data.m_position);
        builder.SetRotation(glm::identity<glm::quat>());
        builder.SetName(data.m_name);

        const glm::vec3 half_extents = glm::abs(data.m_dimensions) * 0.5f;
        builder.RenderAndCollision_SetBox(half_extents, data.m_color);
        if (data.m_has_physics)
        {
            builder.SetPhysicsObjectType(CoreEngine::PhysicsObjectType::Ordinary);
            builder.SetMass(std::max<Units::Kilogram>(data.m_mass, Units::Kilogram(0.0f)));
        }
        m_scene.AddObjectFromBuilder(std::move(builder));

        return true;
    }

    bool Editor_3D_Layer::OnCreateCustomObject(const CustomCreationData& data) noexcept
    {
        Scene3D_ObjectBuilder builder = m_scene.CreateObjectBuilder();

        builder.SetPosition(data.m_position);
        builder.SetRotation(glm::identity<glm::quat>());
        builder.SetName(data.m_name);

        if (! data.m_model_path.empty())
        {
            builder.RenderModel_SetFromPath(data.m_model_path, glm::vec3{1.0f});
        }
        else 
        {
            return false; //Right now Render model MUST exist
        }

        if (data.m_has_physics)
        {
            builder.SetPhysicsObjectType(CoreEngine::PhysicsObjectType::Ordinary);
            builder.CollisionShape_SetBoxFromRenderModelExtents(); //Right now only this, later improve!!
            builder.SetMass(std::max<Units::Kilogram>(data.m_mass, Units::Kilogram(0.0f)));
        }

        m_scene.AddObjectFromBuilder(std::move(builder));

        return true;
    }

    bool Editor_3D_Layer::OnCreateTrianglePointObject(TrianglePointCreationData&& data)
    {
        Scene3D_ObjectBuilder builder = m_scene.CreateObjectBuilder();
        
        builder.SetPosition(data.m_position);
        builder.SetRotation(glm::identity<glm::quat>());
        builder.SetName(data.m_name);

        builder.RenderAndCollision_SetFromPoints(std::move(data.m_points), data.m_color);

        if (data.m_has_physics)
        {
            builder.SetPhysicsObjectType(CoreEngine::PhysicsObjectType::Ordinary);
            builder.SetMass(std::max<Units::Kilogram>(data.m_mass, Units::Kilogram(0.0f)));
        }

        m_scene.AddObjectFromBuilder(std::move(builder));

        return true;
    }

//////////////////////////////////////////////// 
//---------  GUI Rendering & Input
//////////////////////////////////////////////// 
    void Editor_3D_Layer::OnImGuiRender_TopNavigationBar() noexcept
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * TOP_BAR_HEIGHT_RELATIVE));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, BORDER_SIZE_PIXELS);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT);
        ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BACKGROUND);

        ImGui::Begin("Editor_3D_TopNavigationBar", nullptr, window_flags);

        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 2.0f);

        const float line_width = ImGui::GetContentRegionAvail().x;
        const float square_size = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetNextItemWidth(square_size);
        ImGui::SameLine(ImGui::GetCursorPosX() + line_width - square_size - ImGui::GetStyle().ItemSpacing.x);

        if (ImGui::Button("✕", ImVec2(square_size, square_size)))
        {
            GlobalSet<GlobalSet_StopApplication>();
        }

        ImGui::PopFont();

        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    }

    void Editor_3D_Layer::OnImGuiRender_LeftOptionPanel() noexcept
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, BORDER_SIZE_PIXELS);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BACKGROUND);
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip,        COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive,  COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT);

        const float top_bar_height = io.DisplaySize.y * TOP_BAR_HEIGHT_RELATIVE;
        const float panel_height   = io.DisplaySize.y - top_bar_height;
        ImGui::SetNextWindowPos(ImVec2(0, top_bar_height));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * SIDE_PANEL_DEFAULT_WIDTH_RELATIVE, panel_height), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x * SIDE_PANEL_MIN_WIDTH_RELATIVE, panel_height), ImVec2(io.DisplaySize.x * SIDE_PANEL_MAX_WIDTH_RELATIVE, panel_height));

        ImGui::Begin("Editor_3D_LeftOptionPanel", nullptr, window_flags);

        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            if (ImGui::Button("Load##sceneload"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("LoadScene3DDlgKey", "Load Serialized Scene", ".*");
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##scenesave"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("SaveScene3DDlgKey", "Save Serialized Scene", ".*");
            }

            if (ImGuiFileDialog::Instance()->Display("LoadScene3DDlgKey"))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    OnSceneResetAndLoadFromPath(ImGuiFileDialog::Instance()->GetFilePathName());
                }
                ImGuiFileDialog::Instance()->Close();
            }

            if (ImGuiFileDialog::Instance()->Display("SaveScene3DDlgKey"))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    m_scene.SerializeToFile(ImGuiFileDialog::Instance()->GetFilePathName(), m_camera);
                }
                ImGuiFileDialog::Instance()->Close();
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
            if (ImGui::Button("Clear##sceneclear"))
            {
                OnSetSelectedSceneObject(nullptr);
                m_scene.ClearAll();
            }
            ImGui::PopStyleColor();

            if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
                ImGui::Text("Objects: %zu", m_scene.GetAmountObjects());
                ImGui::PopStyleColor();

                if (ImGui::CollapsingHeader("Create##sceneobject", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();

                    if (ImGui::Button("Sphere##create"))
                    {
                        m_object_creation_flags |= ObjectCreationFlag::SPHERE;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Box##create"))
                    {
                        m_object_creation_flags |= ObjectCreationFlag::BOX;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Custom##create"))
                    {
                        m_object_creation_flags |= ObjectCreationFlag::CUSTOM_OBJECT;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Points##create"))
                    {
                        m_object_creation_flags |= ObjectCreationFlag::TRIANGLE_POINTS;
                    }

                    ImGui::Unindent();
                }

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Clear All##sceneobjects"))
                {
                    m_scene.ClearAllSceneObjects();
                }
                ImGui::PopStyleColor();

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Lights##lightsheader", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
                ImGui::Text("Lights: %zu", m_scene.GetLightVectorConstRef().size());
                ImGui::PopStyleColor();

                if (ImGui::Button("Create##lightsource"))
                {
                    m_object_creation_flags |= ObjectCreationFlag::LIGHT_SOURCE;
                }

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Clear All##lightsclear"))
                {
                    m_scene.ClearAllLightSources();
                }
                ImGui::PopStyleColor();

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ENGINE_ASSERT(m_camera_controller && "Camera Controller must not be null.");
                ImGui::Indent();

                //////////////////////////////////////////////// 
                //---------  Select desired camera controller / Free cam if no selected object
                //////////////////////////////////////////////// 
                if (m_selected_object_state.m_object_ptr)
                {
                    int cam_controller = static_cast<int>(m_camera_controller->GetType());
                    ImGui::RadioButton("Free Cam", &cam_controller, static_cast<int>(Basic_CameraController::Type::FreeCam));
                    ImGui::SameLine();
                    ImGui::RadioButton("Orbital" , &cam_controller, static_cast<int>(Basic_CameraController::Type::OrbitalCam)); 
                    ImGui::SameLine();
                    ImGui::RadioButton("Follow"  , &cam_controller, static_cast<int>(Basic_CameraController::Type::FollowCam));
                    if (cam_controller != static_cast<int>(m_camera_controller->GetType()))
                    {
                        switch (static_cast<Basic_CameraController::Type>(cam_controller))
                        {
                            case Basic_CameraController::Type::FreeCam   : OnChangeCameraController<FreeCam_CameraController>();    break;
                            case Basic_CameraController::Type::OrbitalCam: OnChangeCameraController<OrbitalCam_CameraController>(); break;
                            case Basic_CameraController::Type::FollowCam :OnChangeCameraController<FollowCam_CameraController>();   break;
                            default: ENGINE_ASSERT(false && "At OnImGuiRender_LeftOptionPanel(): Expected a valid camera type to be selected.");
                        }
                    }
                }

                //////////////////////////////////////////////// 
                //--------- Options for the selected controller
                //////////////////////////////////////////////// 
                if (m_camera_controller->GetType() == Basic_CameraController::Type::FreeCam)
                {
                    FreeCam_CameraController* controller = static_cast<FreeCam_CameraController*>(m_camera_controller.get());
                    float speed = controller->GetMoveSpeed();
                    ImGui::SliderFloat(":Speed ", &speed, 0.1f, MAX_CAMERA_SPEED);
                    controller->SetMoveSpeed(speed);
                }

                if (m_camera_controller->GetType() == Basic_CameraController::Type::OrbitalCam)
                {
                    OrbitalCam_CameraController* controller = static_cast<OrbitalCam_CameraController*>(m_camera_controller.get());
                    float distance = controller->GetDistance();
                    ImGui::DragFloat(":Distance", &distance, 1.0f, 0.1f, 10'000.0f);
                    controller->SetDistance(distance);
                }

                if (m_camera_controller->GetType() == Basic_CameraController::Type::FollowCam)
                {
                    FollowCam_CameraController* controller = static_cast<FollowCam_CameraController*>(m_camera_controller.get());
                    float distance = controller->GetDistance();
                    ImGui::DragFloat(":Distance", &distance, 1.0f, 0.1f, 10'000.0f);
                    controller->SetDistance(distance);
                }

                float fov = m_camera.GetFovDeg();
                ImGui::SliderFloat(":Fov ", &fov, MIN_FOV_DEGREES, MAX_FOV_DEGREES);
                m_camera.SetFovDeg(fov);

                ImGui::TextUnformatted( m_camera.ToString().c_str() );

                ImGui::Unindent();
            }

            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            
            ImGui::Checkbox("Draw Debug Lines", &m_draw_bullet_debug);

            int flags = m_bt_debug_draw_pipeline.getDebugMode();
            bool wire_frames = flags & btIDebugDraw::DBG_DrawWireframe;
            ImGui::Checkbox("Draw Wire Frames", &wire_frames);
            if (wire_frames) m_bt_debug_draw_pipeline.setDebugMode(flags |= btIDebugDraw::DBG_DrawWireframe);
            else m_bt_debug_draw_pipeline.setDebugMode(flags &= ~btIDebugDraw::DBG_DrawWireframe);

            bool vsync_now = GlobalGet<GlobalGet_VsyncIsOn>();
            ImGui::Checkbox("Toggle Vsync", &vsync_now);
            GlobalSet<GlobalSet_VsyncIsOn>(vsync_now);

            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            ImGui::Checkbox("Update Physics ", &m_update_scene_physics);
            ImGui::SliderFloat("Physics-Speed", &m_physics_speed_scale, 0.01f, 20.0f);

            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            ImGui::Text("FPS: %.0f", io.Framerate);

            ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
            ImGui::TextUnformatted("Logged Frametimes:");
            ImGui::PopStyleColor();

            PerFrameScopeTimes::SortData();
            const std::vector<PerFrameScopeTimes::ScopeTimeData>& scope_times = PerFrameScopeTimes::GetScopeTimeDataConstRef();
            for (auto rit = scope_times.rbegin(); rit != scope_times.rend(); ++rit)
            {
                ImGui::TextUnformatted(rit->ToString().c_str());
            }

            ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
            ImGui::TextUnformatted("Logged Occurences:");
            ImGui::PopStyleColor();

            const std::vector<PerFrameOccurrenceCounter::OccurrenceCounterData>& occurence_counts = PerFrameOccurrenceCounter::GetOccurrenceCounterDataConstRef();
            for (auto rit = occurence_counts.rbegin(); rit != occurence_counts.rend(); ++rit)
            {
                ImGui::TextUnformatted(rit->ToString().c_str());
            }

            ImGui::Unindent();
        }

        m_left_panel_width_relative = ImGui::GetWindowSize().x  / io.DisplaySize.x;
        ImGui::End();

        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);
    }

    void Editor_3D_Layer::OnImGuiRender_RightOptionPanel() noexcept
    { 
        if (!m_selected_object_state.m_object_ptr && (m_object_creation_flags == ObjectCreationFlag::NONE) )
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        //------------ Scaling hell
        const float bottom_height_px = io.DisplaySize.y * m_bottom_panel_height_relative;

        const float top_bar_height_px   = io.DisplaySize.y * TOP_BAR_HEIGHT_RELATIVE;
        const float available_height_px = io.DisplaySize.y - top_bar_height_px - bottom_height_px;

        const float default_width_px = io.DisplaySize.x * SIDE_PANEL_DEFAULT_WIDTH_RELATIVE;
        const float min_width_px     = io.DisplaySize.x * SIDE_PANEL_MIN_WIDTH_RELATIVE;
        const float max_width_px     = io.DisplaySize.x * SIDE_PANEL_MAX_WIDTH_RELATIVE;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, BORDER_SIZE_PIXELS);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BACKGROUND);
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip,        COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive,  COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT);

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - default_width_px, top_bar_height_px), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(default_width_px, available_height_px), ImGuiCond_Appearing);

        ImGui::SetNextWindowSizeConstraints(ImVec2(min_width_px, available_height_px), ImVec2(max_width_px, available_height_px));

        //------------ 
        ImGui::Begin("Editor_3D_RightOptionPanel", nullptr, window_flags);

        ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - ImGui::GetWindowWidth(), top_bar_height_px));
        ImGui::SetWindowSize(ImVec2(ImGui::GetWindowWidth(), available_height_px));

        //------------ Begin actual options
        if (m_object_creation_flags != ObjectCreationFlag::NONE && ImGui::CollapsingHeader("Object Creation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            
            ImGui::PushID(1);
            if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::SPHERE) && ImGui::CollapsingHeader("Creating SPHERE", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("Name", m_sphere_creation_data.m_name, sizeof(m_sphere_creation_data.m_name));

                ImGui::SliderFloat("Radius", &m_sphere_creation_data.m_radius, 0.01f, 1000.0f);
                ImGui::InputFloat3("Position", &m_sphere_creation_data.m_position.x);
                if (ImGui::Button("Set Camera Position"))
                {
                    m_sphere_creation_data.m_position = m_camera.GetPosition();
                }
                ImGui::SliderFloat3("Color", &m_sphere_creation_data.m_color.x, 0.0f, 1.0f);

                ImGui::Checkbox("Has Physics", &m_sphere_creation_data.m_has_physics);

                ImGui::InputFloat("Mass", &m_sphere_creation_data.m_mass.GetReference());

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                if (ImGui::Button("Create"))
                {
                    OnCreateSphere(m_sphere_creation_data);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Cancel"))
                {
                    ObjectCreation_RemoveFlag(m_object_creation_flags, ObjectCreationFlag::SPHERE);
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(2);
            if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::BOX) && ImGui::CollapsingHeader("Creating RECTANGULAR PRISM", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("Name", m_box_creation_data.m_name, sizeof(m_box_creation_data.m_name));

                ImGui::InputFloat3("Size", &m_box_creation_data.m_dimensions.x);
                ImGui::InputFloat3("Position", &m_box_creation_data.m_position.x);
                if (ImGui::Button("Set Camera Position"))
                {
                    m_box_creation_data.m_position = m_camera.GetPosition();
                }
                ImGui::SliderFloat3("Color", &m_box_creation_data.m_color.x, 0.0f, 1.0f);

                ImGui::Checkbox("Has Physics", &m_box_creation_data.m_has_physics);

                ImGui::InputFloat("Mass", &m_box_creation_data.m_mass.GetReference());
                
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                if (ImGui::Button("Create"))
                {
                    OnCreateBox(m_box_creation_data);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Cancel"))
                {
                    ObjectCreation_RemoveFlag(m_object_creation_flags, ObjectCreationFlag::BOX);
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(3);
            if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::CUSTOM_OBJECT) && ImGui::CollapsingHeader("Creating CUSTOM OBJECT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("Name", m_custom_creation_data.m_name, sizeof(m_custom_creation_data.m_name));

                ImGui::InputFloat3("Position", &m_custom_creation_data.m_position.x);
                if (ImGui::Button("Set Camera Position"))
                {
                    m_custom_creation_data.m_position = m_camera.GetPosition();
                }

                if (ImGui::Button("Load Model")) 
                {
                    ImGuiFileDialog::Instance()->OpenDialog("LoadModelDlgKey", "Select Model", ".*");
                }
                if (ImGuiFileDialog::Instance()->Display("LoadModelDlgKey")) 
                {
                    if (ImGuiFileDialog::Instance()->IsOk()) 
                    {
                        m_custom_creation_data.m_model_path = ImGuiFileDialog::Instance()->GetFilePathName();         
                    }
                    ImGuiFileDialog::Instance()->Close();
                }

                ImGui::TextUnformatted("Model: ");
                ImGui::SameLine();

                if (! m_custom_creation_data.m_model_path.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GREEN);
                    ImGui::TextUnformatted(m_custom_creation_data.m_model_path.c_str());
                    ImGui::PopStyleColor();
                }
                else 
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_RED);
                    ImGui::TextUnformatted("NONE");
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Clear Model"))
                {
                    m_custom_creation_data.m_model_path = std::string{};
                }

                //Improve this later
                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
                ImGui::TextUnformatted("---Physics will use Model AABB---");
                ImGui::PopStyleColor();

                ImGui::Checkbox("Has Physics", &m_custom_creation_data.m_has_physics);
                
                ImGui::InputFloat("Mass", &m_custom_creation_data.m_mass.GetReference());

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                if (ImGui::Button("Create"))
                {
                    OnCreateCustomObject(m_custom_creation_data);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Cancel"))
                {
                    ObjectCreation_RemoveFlag(m_object_creation_flags, ObjectCreationFlag::CUSTOM_OBJECT);
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(4);
            if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::TRIANGLE_POINTS) && ImGui::CollapsingHeader("Creating POINT OBJECT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputText("Name", m_triangle_point_creation_data.m_name, sizeof(m_triangle_point_creation_data.m_name));

                ImGui::Checkbox("Has Physics", &m_triangle_point_creation_data.m_has_physics);
                ImGui::SliderFloat3("Color", &m_triangle_point_creation_data.m_color.x, 0.0f, 1.0f);

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
                ImGui::Text("Points: %zu", m_triangle_point_creation_data.m_points.size());
                ImGui::PopStyleColor();

                if (m_next_left_click_places_triangle_point)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                    if (ImGui::Button("Quit Placing Points"))
                    {
                        m_next_left_click_places_triangle_point = false;
                        m_triangle_point_creation_data.m_points.clear();
                    }

                    if (! m_triangle_point_creation_data.m_points.empty())
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("Remove Last Point"))
                        {
                            m_triangle_point_creation_data.m_points.pop_back();
                        }
                    }

                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                    if (ImGui::Button("Begin Placing Points"))
                    {
                        m_next_left_click_places_triangle_point = true;
                        OnSetSelectedSceneObject(nullptr);
                    }
                    ImGui::PopStyleColor();
                }

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                if (ImGui::Button("Create"))
                {
                    if (m_triangle_point_creation_data.m_points.size() >= 3)
                    {
                        const glm::vec3 saved_color  = m_triangle_point_creation_data.m_color;
                        const bool saved_has_physics = m_triangle_point_creation_data.m_has_physics;
                        OnCreateTrianglePointObject(std::move(m_triangle_point_creation_data));
                        m_triangle_point_creation_data = TrianglePointCreationData();
                        m_triangle_point_creation_data.m_color       = saved_color;
                        m_triangle_point_creation_data.m_has_physics = saved_has_physics;
                        m_next_left_click_places_triangle_point = false;
                    }
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Cancel"))
                {
                    ObjectCreation_RemoveFlag(m_object_creation_flags, ObjectCreationFlag::TRIANGLE_POINTS);
                    m_triangle_point_creation_data = TrianglePointCreationData();
                    m_next_left_click_places_triangle_point = false;
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(5);
            if (ObjectCreation_HasFlag(m_object_creation_flags, ObjectCreationFlag::LIGHT_SOURCE) && ImGui::CollapsingHeader("Creating LIGHT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputFloat3("Position", &m_light_source_creation_data.m_position.x);
                if (ImGui::Button("Set Camera Position"))
                {
                    m_light_source_creation_data.m_position = m_camera.GetPosition();
                }
                ImGui::SliderFloat3("Color", &m_light_source_creation_data.m_color.x, 0.0f, 1.0f);

                int mode = m_light_source_creation_data.m_light_mode;
                ImGui::SliderInt("Mode", &mode, Light::LIGHT_MODE::FIRST, Light::LIGHT_MODE::LAST, Light::LightModeToString(mode).c_str());
                m_light_source_creation_data.m_light_mode = mode;

                ImGui::SliderFloat("Intensity", &m_light_source_creation_data.m_intensity, 0.0f, 1000.0f);

                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
                if (ImGui::Button("Create"))
                {
                    m_scene.EmplaceLightSource(m_light_source_creation_data);
                }
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
                if (ImGui::Button("Cancel"))
                {
                    ObjectCreation_RemoveFlag(m_object_creation_flags, ObjectCreationFlag::LIGHT_SOURCE);
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
            
            ImGui::Unindent();
        }

        ImGui::PushID(100);
        if (m_selected_object_state.m_object_ptr && ImGui::CollapsingHeader("Selected Object", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();

            ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ORANGE);
            ImGui::TextUnformatted(("Object: " + m_selected_object_state.m_object_ptr->m_name).c_str());
            ImGui::PopStyleColor();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();

                //--------- Position move
                const float delta_time_secs = GlobalGet<GlobalGet_DeltaTimeSeconds>().Get();
                const float MOVE_SPEED = 0.5 * m_selected_object_state.m_object_ptr->GetWorldSpaceMaxBoundingSphereRadius();

                ImGui::TextUnformatted("Position");

                glm::vec3 position = m_selected_object_state.m_object_ptr->GetPosition();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_X_AXYS);
                if (ImGui::ArrowButton("##xleft", ImGuiDir_Left) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.x -= MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine();
                ImGui::Text("%10.2f", position.x);
                ImGui::SameLine();
                if (ImGui::ArrowButton("##xright", ImGuiDir_Right) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.x += MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine(); 
                ImGui::TextUnformatted("X");
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_Y_AXYS);
                if (ImGui::ArrowButton("##yleft", ImGuiDir_Left) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.y -= MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine();
                ImGui::Text("%10.2f", position.y);
                ImGui::SameLine();
                if (ImGui::ArrowButton("##yright", ImGuiDir_Right) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.y += MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine(); 
                ImGui::TextUnformatted("Y");
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_Z_AXYS);
                if (ImGui::ArrowButton("##zleft", ImGuiDir_Left) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.z -= MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine();
                ImGui::Text("%10.2f", position.z);
                ImGui::SameLine();
                if (ImGui::ArrowButton("##zright", ImGuiDir_Right) || (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) 
                { 
                    position.z += MOVE_SPEED * delta_time_secs; 
                }
                ImGui::SameLine(); 
                ImGui::TextUnformatted("Z");
                ImGui::PopStyleColor();

                //Type position to move to
                if (ImGui::Button("Move to"))
                {
                    position = m_gui_state_selected_object_input_position;
                }
                ImGui::SameLine();
                ImGui::InputFloat3("##position_inputfloat3", &m_gui_state_selected_object_input_position.x);

                if (ImGui::Button("Move to Camera"))
                {
                    position = m_camera.GetPosition();
                }

                if (position != m_selected_object_state.m_object_ptr->GetPosition())
                {
                    m_selected_object_state.m_object_ptr->SetPosition(position);
                }

                //--------- Rotation
                ImGui::TextUnformatted("Rotation");
                const glm::quat rot_quat = m_selected_object_state.m_object_ptr->GetRotation();
                glm::vec3 rot_euler_degrees = glm::degrees(glm::eulerAngles(rot_quat));

                float delta_x = 0.0f, delta_y = 0.0f, delta_z = 0.0f;

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_X_AXYS);
                if (ImGui::DragFloat("##rotx", &delta_x, 0.5f, 0.0f, 0.0f, std::format("X: {:8.2f}°", rot_euler_degrees.x).c_str()))
                {
                    glm::quat delta = glm::angleAxis(glm::radians(delta_x), glm::vec3(1, 0, 0));
                    #ifndef __INTELLISENSE__ //Shut the fuck up intellisense
                    m_selected_object_state.m_object_ptr->SetRotation(rot_quat * delta);
                    #endif
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_Y_AXYS);
                if (ImGui::DragFloat("##roty", &delta_y, 0.5f, 0.0f, 0.0f, std::format("Y: {:8.2f}°", rot_euler_degrees.y).c_str()))
                {
                    glm::quat delta = glm::angleAxis(glm::radians(delta_y), glm::vec3(0, 1, 0));
                    #ifndef __INTELLISENSE__
                    m_selected_object_state.m_object_ptr->SetRotation(rot_quat * delta);
                    #endif
                }
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, COLOR_Z_AXYS);
                if (ImGui::DragFloat("##rotz", &delta_z, 0.5f, 0.0f, 0.0f, std::format("Z: {:8.2f}°", rot_euler_degrees.z).c_str()))
                {
                    glm::quat delta = glm::angleAxis(glm::radians(delta_z), glm::vec3(0, 0, 1));
                    #ifndef __INTELLISENSE__
                    m_selected_object_state.m_object_ptr->SetRotation(rot_quat * delta);
                    #endif
                }
                ImGui::PopStyleColor();

                if (ImGui::Button("Set Identity Rot"))
                {
                    m_selected_object_state.m_object_ptr->SetRotation(glm::identity<glm::quat>());
                }

                glm::vec3 scale = m_selected_object_state.m_object_ptr->GetScale();
                
                if (m_selected_object_state.m_object_ptr->m_physics_object 
                    && m_selected_object_state.m_object_ptr->m_physics_object->GetShapeType() == PhysicsShapeType::SPHERE)
                {
                    ImGui::SliderFloat("Scale##scaleslider1", &scale.x, 0.1f, MAX_OBJ_SCALE_FACTOR);
                    scale.y = scale.z = scale.x;
                }
                else 
                {
                    ImGui::SliderFloat3("Scale##scaleslider3", &scale.x, 0.1f, MAX_OBJ_SCALE_FACTOR);
                }
                if (ImGui::Button("Reset Scale"))
                {
                    scale = glm::vec3(1.0f);
                }
                
                if (scale != m_selected_object_state.m_object_ptr->GetScale())
                {
                    m_selected_object_state.m_object_ptr->SetScale(scale);
                }

                ImGui::Unindent();
            }  

            if (ImGui::CollapsingHeader("Render") && m_selected_object_state.m_object_ptr->m_render_model)
            {
                ImGui::Indent();
                Basic_Model* model = m_selected_object_state.m_object_ptr->m_render_model.get();
                ImGui::Checkbox("Highlight AABB", &m_selected_object_state.m_highlight_aabb_box);
                ImGui::Text("Meshes: %zu", model->GetMeshVectorConstReference().size());
                ImGui::TextUnformatted(("Scale: " + CommonUtility::GlmVec3ToString(model->GetScale())).c_str());
                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Physics") && m_selected_object_state.m_object_ptr->m_physics_object)
            {
                ImGui::Indent();

                PhysicsObject* obj = m_selected_object_state.m_object_ptr->m_physics_object.get();

                bool is_kinematic = obj->IsKinematic();
                ImGui::Checkbox("Is Kinematic", &is_kinematic);

                if (is_kinematic != obj->IsKinematic())
                {
                    obj->SetKinematic(is_kinematic);
                    obj->ResetMotion();
                }

                ImGui::Checkbox("Receive Input", &m_selected_object_state.m_physics_receives_input);

                float mass_now = obj->GetBody()->getMass();
                ImGui::DragFloat("Mass", &mass_now, 1.0f, 0.0f, 100'000.0f, "%.1fkg");
                if (mass_now != obj->GetBody()->getMass())
                {
                    obj->SetWeight(Units::Kilogram(mass_now));
                }

                ImGui::Text("Velocity km/h: %.2f", obj->GetVelocityMS().Get() * 3.6f);

                ImGui::TextUnformatted(("Scale: " + CommonUtility::GlmVec3ToString(PhysicsUtility::BtVector3ToGlm(obj->GetShape()->getLocalScaling()))).c_str());

                ImGui::Unindent();
            }

            ImGui::PushStyleColor(ImGuiCol_Button, COLOR_GREEN);
            if (ImGui::Button("Copy"))
            {
                m_scene.AddObject(std::make_unique<Scene3D_SceneObject>(*m_selected_object_state.m_object_ptr));
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ORANGE);
            if (ImGui::Button("Unselect"))
            {
                OnSetSelectedSceneObject(nullptr);
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            //MUST be last to avoid segfault or a use after free
            ImGui::PushStyleColor(ImGuiCol_Button, COLOR_RED);
            if (ImGui::Button("Delete"))
            {
                Scene3D_SceneObject* obj = m_selected_object_state.m_object_ptr;
                OnSetSelectedSceneObject(nullptr);
                m_scene.RemoveObject(obj);
            }
            ImGui::PopStyleColor();

            ImGui::Unindent();
        }
        ImGui::PopID();

        ImGui::End();
        
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);

    }

    void Editor_3D_Layer::OnImGuiRender_BottomOptionPanel() noexcept
    {
        ImGuiIO& io = ImGui::GetIO();

        const float left_x_pixels      = std::floor(io.DisplaySize.x * m_left_panel_width_relative);
        const float panel_width_pixels = io.DisplaySize.x - left_x_pixels;

        const float default_height_px = io.DisplaySize.y * BOTTOM_PANEL_DEFAULT_HEIGHT_RELATIVE;
        const float min_height_px     = io.DisplaySize.y * BOTTOM_PANEL_MIN_HEIGHT_RELATIVE;
        const float max_height_px     = io.DisplaySize.y * BOTTOM_PANEL_MAX_HEIGHT_RELATIVE;

        const float bottom_y_pos      = io.DisplaySize.y - default_height_px;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, BORDER_SIZE_PIXELS);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BACKGROUND);
        ImGui::PushStyleColor(ImGuiCol_ResizeGrip,        COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive,  COLOR_TRANSPARENT);
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT);

        ImGui::SetNextWindowPos(ImVec2(left_x_pixels, bottom_y_pos), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(panel_width_pixels, default_height_px), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(panel_width_pixels, min_height_px), ImVec2(panel_width_pixels, max_height_px));

        ImGui::Begin("Editor_3D_BottomOptionPanel", nullptr, window_flags);

        ImGui::SetWindowPos(ImVec2(left_x_pixels, io.DisplaySize.y - ImGui::GetWindowHeight()));
        ImGui::SetWindowSize(ImVec2(panel_width_pixels, ImGui::GetWindowHeight()));

        m_bottom_panel_height_relative = ImGui::GetWindowHeight() / io.DisplaySize.y;

        if (m_selected_object_state.m_object_ptr && m_selected_object_state.m_object_ptr->m_render_model)
        {
            const auto DrawImage = [](GLuint id, ImVec2 size) -> void 
            {
                const ImVec2 tex_uv_0 (0, 0);
                const ImVec2 tex_uv_1 (1, 1);
                const ImVec4 tint_col (1, 1, 1, 1);
                ImGui::Image((ImTextureID)(uintptr_t)id, size, tex_uv_0, tex_uv_1, tint_col, COLOR_BORDER);
            };

            const auto DrawPlaceholderX = [](ImVec2 size) -> void 
            {
                ImGui::Dummy(size);
                ImVec2 left_top     (ImGui::GetItemRectMin());
                ImVec2 right_bottom (ImGui::GetItemRectMax());
                ImVec2 left_bottom  (left_top.x, right_bottom.y);
                ImVec2 right_top    (right_bottom.x, left_top.y);
                ImGui::GetWindowDrawList()->AddRect(left_top, right_bottom, IM_COL32(255,0,0,255), 0.0f, ImDrawFlags{0}, BORDER_SIZE_PIXELS);
                ImGui::GetWindowDrawList()->AddLine(left_top, right_bottom, IM_COL32(255, 0, 0, 255));
                ImGui::GetWindowDrawList()->AddLine(left_bottom, right_top, IM_COL32(255, 0, 0, 255));
            };

            const ImVec2 area_available = ImGui::GetContentRegionAvail();
            const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

            const int num_texture_types = 5;
            const float image_width = std::max(50.0f, (area_available.x * 0.8f - (num_texture_types - 1) * spacing.x) / num_texture_types);

            const ImVec2 image_size(image_width, image_width);

            const std::vector<Mesh>& meshes = m_selected_object_state.m_object_ptr->m_render_model->GetMeshVectorConstReference();

            const auto DrawOrPlaceholder = [&](std::shared_ptr<Texture> tex, const char* tooltip_msg)
            {
                ImGui::SameLine();
                if (tex) DrawImage(tex->GetID(), image_size);
                else DrawPlaceholderX(image_size);
                if (ImGui::IsItemHovered()) 
                { 
                    ImGui::SetTooltip(tooltip_msg); 
                }
            };

            for (size_t i = 0; i < meshes.size(); i++)
            {
                const std::shared_ptr<CoreEngine::MaterialPBR> material = meshes[i].GetMaterialConstSharedPtr();
                ENGINE_ASSERT(material && "At Editor_3D_Layer::OnImGuiRender_BottomOptionPanel(): Expected mesh material to not be nullptr.");

                ImGui::TextUnformatted(std::format("Mesh {:>3d}: ", i).c_str());

                #ifndef __INTELLISENSE__
                    DrawOrPlaceholder(material->m_base_texture, "Base Texture");
                    DrawOrPlaceholder(material->m_metallic_roughness_texture, "Metalic Roughness");
                    DrawOrPlaceholder(material->m_normal_texture, "Normal Texture");
                    DrawOrPlaceholder(material->m_occlusion_texture, "Occlusion Texture");
                    DrawOrPlaceholder(material->m_emissive_texture, "Emissive Texture");
                #endif
            }

        }

        ImGui::End();

        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);
    }
}