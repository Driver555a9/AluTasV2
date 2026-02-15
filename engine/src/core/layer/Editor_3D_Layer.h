#pragma once 

//Own includes
#include "core/layer/Freecam_3D_Layer.h"

#include "core/rendering/DrawLines3D_RenderPipeline.h"
#include "core/rendering/DrawPoints3D_RenderPipeline.h"

#include "core/scene/FreeCam_CameraController.h"
#include "core/scene/OrbitalCam_CameraController.h"
#include "core/scene/FollowCam_CameraController.h"
#include "core/scene/DummyCameraController.h"

#include "core/utility/MathUtility.h"
#include "core/utility/Units.h"

//ImGUI
#include "imgui/imgui.h"

namespace CoreEngine
{
    class Editor_3D_Layer : public Freecam_3D_Layer
    {
    public:
        enum class ObjectCreationFlag : std::uint32_t
        {
            NONE                = 0,
            SPHERE              = 1 << 0,
            BOX                 = 1 << 1,
            CUSTOM_OBJECT       = 1 << 2,
            TRIANGLE_POINTS     = 1 << 3,
            LIGHT_SOURCE        = 1 << 4
        };

        Editor_3D_Layer() = default;
        virtual ~Editor_3D_Layer() = default;

        //------- Implementations of Basic_Layers abstract methods
        virtual void OnUpdate(Units::MicroSecond delta_time) override;
        virtual void OnEvent(Basic_Event& event) override;
        virtual void OnRender() override;
        virtual void OnImGuiRender() override;
        //-------

    protected:        
        //////////////////////////////////////////////// 
        //---------  ObjectCreation
        //////////////////////////////////////////////// 
        struct SharedCreationData
        {
            char             m_name[30]    {"Unnamed"};
            glm::vec3        m_position    {};

            Units::Kilogram  m_mass        {0.0f};
            bool             m_has_physics {false};
        };

        struct SphereCreationData : public SharedCreationData
        {
            float     m_radius {1.0f};
            glm::vec3 m_color  {1.0f};
        };

        struct BoxCreationData : public SharedCreationData
        {
            glm::vec3 m_dimensions {1.0f};
            glm::vec3 m_color {1.0f};
        };

        struct CustomCreationData  : public SharedCreationData
        {
            std::string  m_model_path;   

            std::unique_ptr<btCollisionShape>  m_shape = nullptr;
            bool                               m_use_model_AABB_physics_shape {false};
        };

        struct TrianglePointCreationData : public SharedCreationData
        {
            std::vector<Vertex> m_points;

            glm::vec3 m_color {1.0f};
        };

        SphereCreationData           m_sphere_creation_data {};
        BoxCreationData              m_box_creation_data {};
        CustomCreationData           m_custom_creation_data {};
        TrianglePointCreationData    m_triangle_point_creation_data {};
        bool                         m_next_left_click_places_triangle_point = false;

        Light                        m_light_source_creation_data {};

        ObjectCreationFlag           m_object_creation_flags {ObjectCreationFlag::NONE};

        //////////////////////////////////////////////// 
        //---------  Selected scene object
        ////////////////////////////////////////////////
        struct SelectedObjectState
        {
            Scene3D_SceneObject* m_object_ptr = nullptr;
            bool                 m_highlight_aabb_box     {true };
            bool                 m_physics_receives_input {false};
        };
       
        SelectedObjectState m_selected_object_state {};

        //////////////////////////////////////////////// 
        //---------  Selected scene object
        ////////////////////////////////////////////////
        DrawLines3D_RenderPipeline  m_draw_lines_pipeline {};
        DrawPoints3D_RenderPipeline m_draw_points_pipelines {};

        //////////////////////////////////////////////// 
        //---------  Event responses
        ////////////////////////////////////////////////
        [[nodiscard]] virtual bool OnMousePressed(MousePressedEvent& e)   noexcept override;
        [[nodiscard]] virtual bool OnMouseMoved(MouseMovedEvent& e)       noexcept override;
        [[nodiscard]] virtual bool OnMouseScrolled(MouseScrolledEvent& e) noexcept override;

        //////////////////////////////////////////////// 
        //---------  Gui rendering
        ////////////////////////////////////////////////
        void OnImGuiRender_TopNavigationBar()  noexcept;
        void OnImGuiRender_LeftOptionPanel()   noexcept;
        void OnImGuiRender_BottomOptionPanel() noexcept;
        void OnImGuiRender_RightOptionPanel()  noexcept;

        //////////////////////////////////////////////// 
        //---------  Internal helper methods
        ////////////////////////////////////////////////
        [[nodiscard]] MathUtility::Ray3D ScreenPointToRay(const double mouse_x, const double mouse_y) const noexcept;

        bool OnCreateSphere(const SphereCreationData& data) noexcept;
        bool OnCreateBox(const BoxCreationData& data) noexcept;
        bool OnCreateCustomObject(const CustomCreationData& data) noexcept; 
        bool OnCreateTrianglePointObject(TrianglePointCreationData&& data);

        void OnSetSelectedSceneObject(Scene3D_SceneObject* obj) noexcept;
        void OnSceneResetAndLoadFromPath(const std::string& path) noexcept;

        template <typename TCamController>
        requires (std::is_base_of_v<Basic_CameraController, TCamController>)
        constexpr inline void OnChangeCameraController()
        {
            float yaw = 0.0f; 
            float pitch = 0.0f;

            if (m_camera_controller)
            {
                yaw   = m_camera_controller->GetYaw();
                pitch = m_camera_controller->GetPitch();
            }

            if constexpr (std::same_as<TCamController, FreeCam_CameraController>)
            {
                m_camera_controller = std::make_unique<FreeCam_CameraController>();
                m_camera_controller->SetYaw(yaw);
                m_camera_controller->SetPitch(pitch);
            }
            else if constexpr (std::same_as<TCamController, OrbitalCam_CameraController>)
            {
                m_camera_controller = std::make_unique<OrbitalCam_CameraController>();
                m_camera_controller->SetYaw(yaw);
                m_camera_controller->SetPitch(pitch);
                
                if (m_selected_object_state.m_object_ptr)
                {
                    const float selected_obj_bounding_radius = std::max(0.1f, m_selected_object_state.m_object_ptr->GetWorldSpaceMaxBoundingSphereRadius() * 2.0f);
                    static_cast<OrbitalCam_CameraController*>(m_camera_controller.get())->SetDistance(selected_obj_bounding_radius);
                }
            }
            else if constexpr (std::same_as<TCamController, FollowCam_CameraController>)
            {
                m_camera_controller = std::make_unique<FollowCam_CameraController>();
                m_camera_controller->SetYaw(yaw);
                m_camera_controller->SetPitch(pitch);

                if (m_selected_object_state.m_object_ptr)
                {
                    const float selected_obj_bounding_radius = std::max(0.1f, m_selected_object_state.m_object_ptr->GetWorldSpaceMaxBoundingSphereRadius() * 2.0f);
                    static_cast<FollowCam_CameraController*>(m_camera_controller.get())->SetDistance(selected_obj_bounding_radius);
                }
            }
            else if constexpr (std::same_as<TCamController, DummyCameraController>)
            {
                m_camera_controller = std::make_unique<DummyCameraController>();
                m_camera_controller->SetYaw(yaw);
                m_camera_controller->SetPitch(pitch);
            }
            else 
            {
                m_camera_controller = std::make_unique<TCamController>();
            }
        }

        //////////////////////////////////////////////// 
        //---------  GUI States and constants
        //////////////////////////////////////////////// 
        glm::vec3 m_gui_state_selected_object_input_position {0.0f};

        static constexpr const float TOP_BAR_HEIGHT_RELATIVE = 0.04f;

        static constexpr const float SIDE_PANEL_DEFAULT_WIDTH_RELATIVE = 0.2f;
        static constexpr const float SIDE_PANEL_MIN_WIDTH_RELATIVE     = 0.0f;
        static constexpr const float SIDE_PANEL_MAX_WIDTH_RELATIVE     = 0.4f;

        float m_left_panel_width_relative = SIDE_PANEL_DEFAULT_WIDTH_RELATIVE;

        static constexpr const float BOTTOM_PANEL_DEFAULT_HEIGHT_RELATIVE = 0.2f;
        static constexpr const float BOTTOM_PANEL_MIN_HEIGHT_RELATIVE     = 0.0f;
        static constexpr const float BOTTOM_PANEL_MAX_HEIGHT_RELATIVE     = 0.4f;

        float m_bottom_panel_height_relative = BOTTOM_PANEL_MIN_HEIGHT_RELATIVE;

        static constexpr const float BORDER_SIZE_PIXELS   {2.0f};

        static constexpr const float MAX_OBJ_SCALE_FACTOR { 50.0f   };
        static constexpr const float MAX_CAMERA_SPEED     { 2000.0f };
        static constexpr const float MAX_FOV_DEGREES      { 120.0f  };
        static constexpr const float MIN_FOV_DEGREES      { 20.0f   };

        static constexpr const ImVec4 COLOR_RED          {0.8f, 0, 0, 1.0f};
        static constexpr const ImVec4 COLOR_GREEN        {0, 0.8f, 0, 1};
        static constexpr const ImVec4 COLOR_BLUE         {0, 0, 0.8f, 1};
        static constexpr const ImVec4 COLOR_ORANGE       {1.0f, 0.518f, 0.0f, 1.0f};
        static constexpr const ImVec4 COLOR_TRANSPARENT  {0, 0, 0, 0};

        static constexpr const ImVec4 COLOR_BACKGROUND   {0.1f, 0.1f, 0.1f, 1.0f};
        static constexpr const ImVec4 COLOR_BORDER       {0.5f, 0.5f, 0.5f, 0.8f};
        static constexpr const ImVec4 COLOR_TEXT         {1, 1, 1, 1};

        static constexpr const ImVec4 COLOR_X_AXYS      { COLOR_RED   };
        static constexpr const ImVec4 COLOR_Y_AXYS      {0.f, 0.796f, 1.f, 1.0f};
        static constexpr const ImVec4 COLOR_Z_AXYS      { COLOR_GREEN };
    };

    constexpr Editor_3D_Layer::ObjectCreationFlag operator|(const Editor_3D_Layer::ObjectCreationFlag lhs, const Editor_3D_Layer::ObjectCreationFlag rhs) noexcept
    {
        return static_cast<Editor_3D_Layer::ObjectCreationFlag>(static_cast<const std::uint32_t>(lhs) | static_cast<const std::uint32_t>(rhs));
    }

    constexpr Editor_3D_Layer::ObjectCreationFlag operator&(const Editor_3D_Layer::ObjectCreationFlag lhs, const Editor_3D_Layer::ObjectCreationFlag rhs) noexcept
    {
        return static_cast<Editor_3D_Layer::ObjectCreationFlag>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    constexpr Editor_3D_Layer::ObjectCreationFlag operator~(const Editor_3D_Layer::ObjectCreationFlag flag) noexcept
    {
        return static_cast<Editor_3D_Layer::ObjectCreationFlag>(~static_cast<const std::uint32_t>(flag));
    }

    constexpr Editor_3D_Layer::ObjectCreationFlag& operator|=(Editor_3D_Layer::ObjectCreationFlag& lhs, const Editor_3D_Layer::ObjectCreationFlag rhs) noexcept
    {
        return lhs = lhs | rhs;
    }

    constexpr Editor_3D_Layer::ObjectCreationFlag& operator&=(Editor_3D_Layer::ObjectCreationFlag& lhs, const Editor_3D_Layer::ObjectCreationFlag rhs) noexcept
    {
        return lhs = lhs & rhs;
    }

    [[nodiscard]] constexpr bool ObjectCreation_HasFlag(const Editor_3D_Layer::ObjectCreationFlag value, Editor_3D_Layer::ObjectCreationFlag&& flag) noexcept
    {
        return (value & flag) != Editor_3D_Layer::ObjectCreationFlag::NONE;
    }

    constexpr void ObjectCreation_RemoveFlag(Editor_3D_Layer::ObjectCreationFlag& value, Editor_3D_Layer::ObjectCreationFlag&& flag) noexcept 
    {
        value &= ~flag;
    }
}