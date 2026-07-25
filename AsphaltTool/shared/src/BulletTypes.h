#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <format>

namespace BulletTypes
{
    ////////////////////////////////////////////
    // Primitives
    ////////////////////////////////////////////
    struct UnalignedVector3
    {
        float x{}, y{}, z{};
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        [[nodiscard]] UnalignedVector3 operator+(const UnalignedVector3& other) const { return {x + other.x, y + other.y, z + other.z};}
        [[nodiscard]] UnalignedVector3 operator-(const UnalignedVector3& other) const { return {x - other.x, y - other.y, z - other.z};}
        [[nodiscard]] UnalignedVector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
        [[nodiscard]] UnalignedVector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
        [[nodiscard]] UnalignedVector3 operator-() const { return {-x, -y, -z}; }
        UnalignedVector3& operator+=(const UnalignedVector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
        UnalignedVector3& operator-=(const UnalignedVector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
        UnalignedVector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        UnalignedVector3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
        [[nodiscard]] bool operator==(const UnalignedVector3& other) const { return x == other.x && y == other.y && z == other.z; }
        [[nodiscard]] bool operator!=(const UnalignedVector3& other) const { return !(*this == other); }
        float& operator[](size_t i) { assert(i < 3); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 3); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f})", x, y, z); }
    };
    static_assert(sizeof(UnalignedVector3) == 12);

    struct alignas(16) Vector3
    {
        float x{}, y{}, z{};
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        [[nodiscard]] Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z};}
        [[nodiscard]] Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z};}
        [[nodiscard]] Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
        [[nodiscard]] Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
        [[nodiscard]] Vector3 operator-() const { return {-x, -y, -z}; }
        Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
        Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
        Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        Vector3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
        [[nodiscard]] bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
        [[nodiscard]] bool operator!=(const Vector3& other) const { return !(*this == other); }
        float& operator[](size_t i) { assert(i < 3); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 3); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f})", x, y, z); }
    };
    static_assert(sizeof(Vector3) == 16);

    struct UnalignedVector4 
    { 
        float x{}, y{}, z{}, w{}; 
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        float& operator[](size_t i) { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(UnalignedVector4) == 16);

    struct alignas(16) Vector4 
    { 
        float x{}, y{}, z{}, w{}; 
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        float& operator[](size_t i) { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(Vector4) == 16);

    struct UnalignedTransform
    {
        UnalignedVector4 m_basis[3];
        UnalignedVector4 m_origin;

        float& operator[](size_t i) { assert(i < 16); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 16); return reinterpret_cast<const float*>(this)[i]; }
        float* Data() noexcept { return reinterpret_cast<float*>(this); }
        const float* Data() const noexcept { return reinterpret_cast<const float*>(this); }
        [[nodiscard]] std::string ToString() const
        {
            return std::format(
                "[{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                (*this)[0],  (*this)[1],  (*this)[2],  (*this)[3],
                (*this)[4],  (*this)[5],  (*this)[6],  (*this)[7],
                (*this)[8],  (*this)[9],  (*this)[10], (*this)[11],
                (*this)[12], (*this)[13], (*this)[14], (*this)[15]
            );
        }
    };
    static_assert(sizeof(UnalignedTransform) == 16 * sizeof(float));

    struct alignas(16) Transform
    {
        Vector4 m_basis[3];
        Vector4 m_origin;
        
        float& operator[](size_t i) { assert(i < 16); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 16); return reinterpret_cast<const float*>(this)[i]; }
        float* Data() noexcept { return reinterpret_cast<float*>(this); }
        const float* Data() const noexcept { return reinterpret_cast<const float*>(this); }
        [[nodiscard]] std::string ToString() const
        {
            return std::format(
                "[{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                (*this)[0],  (*this)[1],  (*this)[2],  (*this)[3],
                (*this)[4],  (*this)[5],  (*this)[6],  (*this)[7],
                (*this)[8],  (*this)[9],  (*this)[10], (*this)[11],
                (*this)[12], (*this)[13], (*this)[14], (*this)[15]
            );
        }
    };
    static_assert(sizeof(Transform) == 16 * sizeof(float));

    struct UnalignedQuaternion
    {
        float x, y, z, w;
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        float& operator[](size_t i) { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(UnalignedQuaternion) == 4 * sizeof(float));

    struct alignas(16) Quaternion
    {
        float x, y, z, w;
        [[nodiscard]] float* Data() { return &x; }
        [[nodiscard]] const float* Data() const { return &x; }
        float& operator[](size_t i) { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(Quaternion) == 4 * sizeof(float));

    template<typename T>
    struct AlignedObjectArray
    {
        T*       m_data {};
        int32_t  m_size {};
        int32_t  m_capacity {};
    };
    static_assert(sizeof(AlignedObjectArray<void*>) == 0x10);

    ////////////////////////////////////////////
    // Complex physics types
    ////////////////////////////////////////////
    struct RaycastOutput
    {
        void*            m_hit_body_ptr {};
        UnalignedVector3 m_hit_position {};
        UnalignedVector3 m_hit_normal {};
        int32_t          m_activation_filter {};
        uint32_t         m_unkown_4 {};
        uint64_t         m_unkown_8 {};
        bool             m_has_hit {};
    };
    static_assert(offsetof(RaycastOutput, m_has_hit) == 0x30);

    struct CollisionShapeData
    {
        void*    m_vtable_ptr {};
        uint8_t  m_unkown_bytes_48[48];
        int      m_v10_type {};
    };
    static_assert(offsetof(CollisionShapeData, m_v10_type) == 0x38);

    //fwd
    struct BroadphaseProxy;

    enum CollisionFlags
	{
		CF_DYNAMIC_OBJECT                   = 0,
		CF_STATIC_OBJECT                    = 1,
		CF_KINEMATIC_OBJECT                 = 2,
		CF_NO_CONTACT_RESPONSE              = 4,
		CF_CUSTOM_MATERIAL_CALLBACK         = 8,
		CF_CHARACTER_OBJECT                 = 16,
		CF_DISABLE_VISUALIZE_OBJECT         = 32,
		CF_DISABLE_SPU_COLLISION_PROCESSING = 64,
		CF_HAS_CONTACT_STIFFNESS_DAMPING    = 128,
		CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR = 256,
		CF_HAS_FRICTION_ANCHOR              = 512,
		CF_HAS_COLLISION_SOUND_TRIGGER      = 1024
	};

	enum CollisionObjectTypes
	{
		CO_COLLISION_OBJECT  = 1,
		CO_RIGID_BODY        = 2,
		CO_GHOST_OBJECT      = 4,
		CO_SOFT_BODY         = 8,
		CO_HF_FLUID          = 16,
		CO_USER_TYPE         = 32,
		CO_FEATHERSTONE_LINK = 64
	};

	enum AnisotropicFrictionFlags
	{
		CF_ANISOTROPIC_FRICTION_DISABLED = 0,
		CF_ANISOTROPIC_FRICTION = 1,
		CF_ANISOTROPIC_ROLLING_FRICTION = 2
	};

    struct alignas(16) CollisionObject
    {
        void*               m_vtable_ptr {};
        uint8_t             m_header_padding[24];
        Transform           m_transform_matrix;
        Transform           m_interpolation_world_transform;
        Vector3             m_interpolation_linear_velocity;
        Vector3             m_interpolation_angular_velocity;
        Vector3             m_anisotropic_friction;
        int32_t             m_has_anisotropic_friction;
        float               m_contact_processing_threshold;
        BroadphaseProxy*    m_broadphase_proxy_ptr;
        CollisionShapeData* m_collision_shape_ptr;
        void*               m_extensionPointer;
        CollisionShapeData* m_root_collision_shape_ptr;
        int                 m_collision_flags;
        int                 m_island_tag_1;
        int                 m_companion_id;
        int                 m_world_array_index;
        int                 m_activation_state_1;
        float               m_deactivation_time;
        float               m_friction;
        float               m_restitution;
        float               m_rolling_friction;
        float               m_spinning_friction;
        float               m_contact_damping;
        float               m_contact_stiffness;
        int                 m_internal_type;
        void*               m_user_object_pointer;
        int                 m_user_index_2;
        int                 m_user_index;
        int                 m_user_index_3;
        float               m_hit_fraction;
        float               m_ccd_swept_sphere_radius;
        float               m_ccd_motion_threshold;
        int                 m_check_collide_with;
        AlignedObjectArray<const CollisionObject*> m_objects_without_collision_check;
        int                 m_update_revision;
        Vector3             m_custom_debug_color_RGB;
    };
    static_assert(offsetof(CollisionObject, m_transform_matrix)     == 0x20);
    static_assert(offsetof(CollisionObject, m_broadphase_proxy_ptr) == 0xD8);
    static_assert(offsetof(CollisionObject, m_collision_shape_ptr)  == 0xE0);

    struct alignas(16) BroadphaseProxy
    {
        CollisionObject*       m_client_body {}; // btCollisionObject or Rigidbody
        int                    m_collision_filter_group;
        int                    m_collision_filter_mask;
        int                    m_unique_id;
        Vector3   m_aabb_min;
        Vector3   m_aabb_max;
    };
    static_assert(offsetof(BroadphaseProxy, m_aabb_min) == 0x20);
    static_assert(offsetof(BroadphaseProxy, m_aabb_max) == 0x30);

}