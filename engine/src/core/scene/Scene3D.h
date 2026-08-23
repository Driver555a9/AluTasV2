#pragma once

//Own includes
#include "core/model/Model.h"
#include "core/scene/Scene3D_SceneObject.h"
#include "core/scene/Scene3D_ObjectBuilder.h"

#include "core/utility/MathUtility.h"
#include "core/utility/Units.h"

#include "core/scene/Camera.h"

#include <cstdint>
#include <unordered_map>

namespace CoreEngine
{
    class Scene3D final
    {   
    public:
        struct ObjectID
        {
            uint32_t m_index{ 0xFFFFFFFF };
            uint32_t m_generation{ 0 };

            [[nodiscard]] bool IsValid() const noexcept { return m_index != 0xFFFFFFFF; }
            
            bool operator==(const ObjectID& other) const noexcept
            {
                return m_index == other.m_index && m_generation == other.m_generation;
            }
            bool operator!=(const ObjectID& other) const noexcept { return !(*this == other); }

            static ObjectID CreateInvalidID() { return ObjectID { .m_index = 0xFFFFFFFF, .m_generation = 0}; }
        };

        struct Slot
        {
            std::unique_ptr<Scene3D_SceneObject> m_object{ nullptr };
            uint32_t m_generation{ 0 };
        };
        
        explicit Scene3D() noexcept = default;

        //////////////////////////////////////////////// 
        //--------- Update logic
        //////////////////////////////////////////////// 

        void OnPhysicsUpdate(const Units::MicroSecond delta_time) noexcept;
        
        void OnDrawBtDebug() noexcept;
        void SetDebugDrawer(btIDebugDraw* drawer) noexcept;

        [[nodiscard]] const std::vector<const Basic_Model*> GetRenderModelVector() const noexcept;
        [[nodiscard]] const std::vector<Light>& GetLightVectorConstRef() const noexcept;
        [[nodiscard]] std::vector<glm::vec3> GetDebugLinesAllObjects() const noexcept;
        [[nodiscard]] std::vector<Slot>& GetSlotVectorRef() noexcept;

        //////////////////////////////////////////////// 
        //--------- Adding / Deleting objects or lights
        //////////////////////////////////////////////// 

        [[nodiscard]] Scene3D_ObjectBuilder CreateObjectBuilder() noexcept;
        ObjectID AddObject(std::unique_ptr<Scene3D_SceneObject> obj) noexcept;
        ObjectID AddObjectFromBuilder(Scene3D_ObjectBuilder&& builder) noexcept;
        [[nodiscard]] Scene3D_SceneObject* GetSceneObject(ObjectID handle) const noexcept;
        bool RemoveObject(ObjectID handle) noexcept;
        
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

        [[nodiscard]] size_t GetAmountObjects() const noexcept;

        //////////////////////////////////////////////// 
        //--------- Serialization
        //////////////////////////////////////////////// 

        /// Very limited and must be improved
        bool SerializeToFile(const std::string& file_path, const CameraReverseZ& camera) const;
        [[nodiscard]] std::string SerializeToString(const CameraReverseZ& camera) const;

        void LoadFromSerializedFile(const std::string& file_path, std::optional<CameraReverseZ*> opt_camera);
        void LoadFromSerializedString(const std::string& data, std::optional<CameraReverseZ*> opt_camera);

        //////////////////////////////////////////////// 
        //--------- Raycasting
        //////////////////////////////////////////////// 
        struct RaycastHit 
        {
            Scene3D::ObjectID m_scene_object_id = {};
            glm::vec3    m_intersection_point {};
            glm::vec3    m_normal {};
            [[nodiscard]] bool HasHit() const noexcept { return m_scene_object_id.IsValid(); }
        };

        [[nodiscard]] RaycastHit RaycastSelect(const MathUtility::Ray3D& ray) noexcept;

        //////////////////////////////////////////////// 
        //--------- Copy / Move policy
        //////////////////////////////////////////////// 
        Scene3D& operator=(Scene3D&&)      = default;
        Scene3D(Scene3D&&)                 = default;

        Scene3D& operator=(const Scene3D&) = delete;
        Scene3D(const Scene3D&)            = delete;

    private:
        //////////////////////////////////////////////// 
        //---------  Members
        //////////////////////////////////////////////// 
        PhysicsWorld                                      m_physics_world; //Needs to die *after* scene_objects die

        std::vector<Slot> m_slots;
        std::vector<uint32_t> m_free_indices;
        int64_t m_object_count = 0;

        std::vector<Light>                                m_light_sources;

        bool                                              m_object_added_or_deleted = false;
        bool                                              m_light_added_or_deleted  = false;
    };

}