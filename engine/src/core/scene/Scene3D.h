#pragma once

//Own includes
#include "core/scene/Scene3D_SceneObject.h"
#include "core/scene/Scene3D_ObjectBuilder.h"

#include "core/utility/MathUtility.h"
#include "core/utility/Units.h"

#include "core/scene/Camera.h"

namespace CoreEngine
{
    class Scene3D final
    {   
    public:
    
        explicit Scene3D() noexcept = default;

        //////////////////////////////////////////////// 
        //--------- Update logic
        //////////////////////////////////////////////// 

        void OnPhysicsUpdate(const Units::MicroSecond delta_time) noexcept;
        
        void OnDrawBtDebug() noexcept;
        void SetDebugDrawer(btIDebugDraw* drawer) noexcept;

        [[nodiscard]] const std::vector<Basic_Model*> GetRenderModelVector() const noexcept;
        [[nodiscard]] const std::vector<Light>& GetLightVectorConstRef() const noexcept;
        [[nodiscard]] std::vector<glm::vec3> GetDebugLinesAllObjects() const noexcept;
        [[nodiscard]] std::vector<std::unique_ptr<Scene3D_SceneObject>>& GetSceneObjectsRef() noexcept;

        //////////////////////////////////////////////// 
        //--------- Adding / Deleting objects or lights
        //////////////////////////////////////////////// 

        void AddObject(std::unique_ptr<Scene3D_SceneObject> obj) noexcept;

        [[nodiscard]] Scene3D_ObjectBuilder CreateObjectBuilder() noexcept;
        void AddObjectFromBuilder(Scene3D_ObjectBuilder&& builder) noexcept;

        bool RemoveObject(const Scene3D_SceneObject* object_ptr) noexcept;
        bool RemoveObject(const std::size_t index) noexcept;
        
        [[nodiscard]] bool GetAndResetObjectVecChangeFlag () noexcept;
        [[nodiscard]] bool GetAndResetLightVecChangeFlag () noexcept;

        template <typename... Args>
        requires ( std::is_constructible_v<Light, Args...> )
        inline void EmplaceLightSource(Args&&... args) noexcept
        {
            m_light_sources.emplace_back(std::forward<Args>(args)...);
            m_light_added_or_deleted = true;
        }

        bool RemoveLightSource(const std::size_t index) noexcept;

        void ClearAllLightSources() noexcept;
        void ClearAllSceneObjects() noexcept;
        void ClearAll() noexcept;

        [[nodiscard]] size_t GetAmountObjects() noexcept;

        //////////////////////////////////////////////// 
        //--------- Serialization
        //////////////////////////////////////////////// 

        /// Very limited and must be improved
        bool SerializeToFile(const std::string& file_path, const CameraReverseZ& camera) const;
        [[nodiscard]] std::string SerializeToString(const CameraReverseZ& camera) const;

        CameraReverseZ LoadFromSerializedFile(const std::string& file_path);
        CameraReverseZ LoadFromSerializedString(const std::string& data);

        //////////////////////////////////////////////// 
        //--------- Raycasting
        //////////////////////////////////////////////// 
        struct RaycastHit 
        {
            Scene3D_SceneObject* m_scene_object_ptr = nullptr;
            glm::vec3    m_intersection_point {};
            glm::vec3    m_normal {};
        };

        [[nodiscard]] RaycastHit RaycastSelect(const MathUtility::Ray3D& ray) noexcept;

        //////////////////////////////////////////////// 
        //--------- Copy / Move policy
        //////////////////////////////////////////////// 
        Scene3D& operator=(Scene3D&&)      = default;
        Scene3D(Scene3D&&)                 = default;

        Scene3D& operator=(const Scene3D&) = delete;
        Scene3D(const Scene3D&)            = delete;

    protected:
        //////////////////////////////////////////////// 
        //---------  Members
        //////////////////////////////////////////////// 
        PhysicsWorld                                      m_physics_world; //Needs to die *after* scene_objects die
        std::vector<std::unique_ptr<Scene3D_SceneObject>> m_scene_objects;  
        std::vector<Light>                                m_light_sources;

        bool                                              m_object_added_or_deleted = false;
        bool                                              m_light_added_or_deleted  = false;
    };

}