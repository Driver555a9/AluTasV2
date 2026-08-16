#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <format>
#include <type_traits>

namespace BulletTypes
{
    ////////////////////////////////////////////
    // Primitives
    ////////////////////////////////////////////
    struct UnalignedVector3
    {
        float x{}, y{}, z{};
        UnalignedVector3() noexcept : x(), y(), z() {}
        UnalignedVector3(float uniform_value) noexcept : x(uniform_value), y(uniform_value), z(uniform_value) {}
        UnalignedVector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
        [[nodiscard]] float* Data() noexcept { return &x; }
        [[nodiscard]] const float* Data() const noexcept { return &x; }
        [[nodiscard]] UnalignedVector3 operator+(const UnalignedVector3& other) const noexcept { return {x + other.x, y + other.y, z + other.z};}
        [[nodiscard]] UnalignedVector3 operator-(const UnalignedVector3& other) const noexcept { return {x - other.x, y - other.y, z - other.z};}
        [[nodiscard]] UnalignedVector3 operator*(float scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }
        [[nodiscard]] UnalignedVector3 operator/(float scalar) const noexcept { return {x / scalar, y / scalar, z / scalar}; }
        [[nodiscard]] UnalignedVector3 operator-() const noexcept { return {-x, -y, -z}; }
        UnalignedVector3& operator+=(const UnalignedVector3& other) noexcept { x += other.x; y += other.y; z += other.z; return *this; }
        UnalignedVector3& operator-=(const UnalignedVector3& other) noexcept { x -= other.x; y -= other.y; z -= other.z; return *this; }
        UnalignedVector3& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
        UnalignedVector3& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }
        [[nodiscard]] bool operator==(const UnalignedVector3& other) const noexcept { return x == other.x && y == other.y && z == other.z; }
        [[nodiscard]] bool operator!=(const UnalignedVector3& other) const noexcept { return !(*this == other); }
        float& operator[](size_t i) noexcept { assert(i < 3); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const noexcept { assert(i < 3); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] std::string ToString() const noexcept { return std::format("({:.3f}, {:.3f}, {:.3f})", x, y, z); }
    };
    static_assert(sizeof(UnalignedVector3) == 12);

    struct alignas(16) Vector3
    {
        float x{}, y{}, z{};
        Vector3() noexcept : x(), y(), z() {}
        Vector3(float uniform_value) : x(uniform_value), y(uniform_value), z(uniform_value) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
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
        [[nodiscard]] float* Data() noexcept { return &x; }
        [[nodiscard]] const float* Data() const noexcept { return &x; }
        float& operator[](size_t i) noexcept { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const noexcept { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] bool operator==(const UnalignedVector4& other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
        [[nodiscard]] bool operator!=(const UnalignedVector4& other) const noexcept { return !(*this == other); }
        [[nodiscard]] std::string ToString() const noexcept { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(UnalignedVector4) == 16);

    struct alignas(16) Vector4 
    { 
        float x{}, y{}, z{}, w{}; 
        [[nodiscard]] float* Data() noexcept { return &x; }
        [[nodiscard]] const float* Data() const noexcept { return &x; }
        float& operator[](size_t i) noexcept { assert(i < 4); return reinterpret_cast<float*>(this)[i]; }
        const float& operator[](size_t i) const noexcept { assert(i < 4); return reinterpret_cast<const float*>(this)[i]; }
        [[nodiscard]] bool operator==(const Vector4& other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
        [[nodiscard]] bool operator!=(const Vector4& other) const noexcept { return !(*this == other); }
        [[nodiscard]] std::string ToString() const noexcept { return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", x, y, z, w); }
    };
    static_assert(sizeof(Vector4) == 16);

    struct alignas(16) Matrix3x3 
    {
        Vector3 m_rows[3];
    };
    static_assert(sizeof(Matrix3x3) == 48);

    struct UnalignedTransform
    {
        UnalignedVector4 m_basis[3];
        UnalignedVector4 m_origin;
        UnalignedTransform() noexcept : m_basis({1, 0, 0}, {0, 1, 0}, {0, 0, 1}), m_origin(0, 0, 0) {}
        UnalignedVector4& operator[](size_t col) noexcept { assert(col < 4); return (col < 3) ? m_basis[col] : m_origin; }
        const UnalignedVector4& operator[](size_t col) const noexcept { assert(col < 4);  return (col < 3) ? m_basis[col] : m_origin; }
        float& At(size_t flat_index) noexcept { assert(flat_index < 16); return reinterpret_cast<float*>(this)[flat_index]; }
        const float& At(size_t flat_index) const noexcept { assert(flat_index < 16); return reinterpret_cast<const float*>(this)[flat_index]; }
        float* Data() noexcept { return reinterpret_cast<float*>(this); }
        const float* Data() const noexcept { return reinterpret_cast<const float*>(this); }
        [[nodiscard]] bool operator==(const UnalignedTransform& other) const noexcept
        { 
            return m_basis[0] == other.m_basis[0] && m_basis[1] == other.m_basis[1] && m_basis[2] == other.m_basis[2] && m_origin == other.m_origin;
        }
        [[nodiscard]] bool operator!=(const UnalignedTransform& other) const noexcept { return !(*this == other); }
        [[nodiscard]] std::string ToString() const noexcept
        {
            return std::format(
                "[{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                (*this)[0].x, (*this)[0].y, (*this)[0].z, (*this)[0].w,
                (*this)[1].x, (*this)[1].y, (*this)[1].z, (*this)[1].w,
                (*this)[2].x, (*this)[2].y, (*this)[2].z, (*this)[2].w,
                (*this)[3].x, (*this)[3].y, (*this)[3].z, (*this)[3].w
            );
        }
    };
    static_assert(sizeof(UnalignedTransform) == 16 * sizeof(float));

    struct alignas(16) Transform
    {
        Vector4 m_basis[3];
        Vector4 m_origin;
        Transform() noexcept : m_basis({1, 0, 0}, {0, 1, 0}, {0, 0, 1}), m_origin(0, 0, 0) {}
        Vector4& operator[](size_t col) noexcept{ assert(col < 4); return (col < 3) ? m_basis[col] : m_origin; }
        const Vector4& operator[](size_t col) const noexcept { assert(col < 4); return (col < 3) ? m_basis[col] : m_origin; }
        float& At(size_t flat_index) noexcept { assert(flat_index < 16); return reinterpret_cast<float*>(this)[flat_index]; }
        const float& At(size_t flat_index) const noexcept { assert(flat_index < 16); return reinterpret_cast<const float*>(this)[flat_index]; }
        float* Data() noexcept { return reinterpret_cast<float*>(this); }
        const float* Data() const noexcept { return reinterpret_cast<const float*>(this); }
        [[nodiscard]] bool operator==(const Transform& other) const noexcept
        { 
            return m_basis[0] == other.m_basis[0] && m_basis[1] == other.m_basis[1] && m_basis[2] == other.m_basis[2] && m_origin == other.m_origin;
        }
        [[nodiscard]] bool operator!=(const Transform& other) const noexcept { return !(*this == other); }
        [[nodiscard]] std::string ToString() const noexcept
        {
            return std::format(
                "[{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}, "
                "{:.3f}, {:.3f}, {:.3f}, {:.3f}]",
                (*this)[0].x, (*this)[0].y, (*this)[0].z, (*this)[0].w,
                (*this)[1].x, (*this)[1].y, (*this)[1].z, (*this)[1].w,
                (*this)[2].x, (*this)[2].y, (*this)[2].z, (*this)[2].w,
                (*this)[3].x, (*this)[3].y, (*this)[3].z, (*this)[3].w
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

    #pragma pack(push, 1)
    template <typename T>
    struct AlignedObjectArray
    {
        int     m_size;
        int     m_capacity;
        uint8_t m_unknown[4];
        T*      m_data;
        bool    m_owns_memory;
        uint8_t m_tail_pad[7]; 
        T& operator[](int32_t i) { assert(i < m_size && m_size >= 0); return m_data[i]; }
        const T& operator[](int32_t i) const { assert(i < m_size && m_size >= 0); return m_data[i]; }
    };
    #pragma pack(pop)
    static_assert(sizeof(AlignedObjectArray<void*>) == 28);

    ////////////////////////////////////////////
    // Complex physics types
    ////////////////////////////////////////////

    struct RaycastOutput
    {
        void*            m_hit_body_ptr {}; // Not a Collision object? Unclear what it points to
        UnalignedVector3 m_hit_position {};
        UnalignedVector3 m_hit_normal {};
        int32_t          m_activation_filter {};
        uint32_t         m_unkown_4 {};
        uint64_t         m_unkown_8 {};
        bool             m_has_hit {};
    };
    static_assert(offsetof(RaycastOutput, m_has_hit) == 0x30);

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

    enum BroadphaseNativeTypes
    {
        BOX_SHAPE_PROXYTYPE,                           // 0 - USED IN GAME
        TRIANGLE_SHAPE_PROXYTYPE,
        TETRAHEDRAL_SHAPE_PROXYTYPE,
        CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE,
        CONVEX_HULL_SHAPE_PROXYTYPE,
        CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE,
        CUSTOM_POLYHEDRAL_SHAPE_TYPE,
        IMPLICIT_CONVEX_SHAPES_START_HERE,
        SPHERE_SHAPE_PROXYTYPE,                        // 8 - USED IN GAME
        MULTI_SPHERE_SHAPE_PROXYTYPE,
        CAPSULE_SHAPE_PROXYTYPE,                       // 10 - USED IN GAME (Inside Compound)
        CONE_SHAPE_PROXYTYPE,
        CONVEX_SHAPE_PROXYTYPE,
        CYLINDER_SHAPE_PROXYTYPE,                      // 13 - Used in game (see rome)
        UNIFORM_SCALING_SHAPE_PROXYTYPE,
        MINKOWSKI_SUM_SHAPE_PROXYTYPE,
        MINKOWSKI_DIFFERENCE_SHAPE_PROXYTYPE,
        BOX_2D_SHAPE_PROXYTYPE,
        CONVEX_2D_SHAPE_PROXYTYPE,
        CUSTOM_CONVEX_SHAPE_TYPE,
        CONCAVE_SHAPES_START_HERE,
        TRIANGLE_MESH_SHAPE_PROXYTYPE,
        SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE,           // 22 - USED IN GAME
        FAST_CONCAVE_MESH_PROXYTYPE,
        TERRAIN_SHAPE_PROXYTYPE,
        GIMPACT_SHAPE_PROXYTYPE,
        MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE,          // 26 - USED IN GAME
        EMPTY_SHAPE_PROXYTYPE,
        STATIC_PLANE_PROXYTYPE,
        CUSTOM_CONCAVE_SHAPE_TYPE,
        SDF_SHAPE_PROXYTYPE = CUSTOM_CONCAVE_SHAPE_TYPE,
        CONCAVE_SHAPES_END_HERE,
        COMPOUND_SHAPE_PROXYTYPE,                        // 31 - USED IN GAME
        SOFTBODY_SHAPE_PROXYTYPE,
        HFFLUID_SHAPE_PROXYTYPE,
        HFFLUID_BUOYANT_CONVEX_SHAPE_PROXYTYPE,
        INVALID_SHAPE_PROXYTYPE,
        MAX_BROADPHASE_COLLISION_TYPES
    };

    enum class PHY_ScalarType
    {
        PHY_FLOAT,
        PHY_DOUBLE,
        PHY_INTEGER,
        PHY_SHORT,
        PHY_FIXEDPOINT88,
        PHY_UCHAR
    };

    struct alignas(16) CollisionShape
    {
        void**   m_vtable_ptr {};
        uint8_t  m_padding[48];     // Added with respect to game ABI
        int      m_shape_type {};   // See BroadphaseNativeType 
        void*    m_user_pointer {};
        int      m_user_index {};
        int      m_user_index2 {};

        ////////////////////////////////////
        // Dynamic dispatch methods
        ////////////////////////////////////
        void GetAABB(Transform& transform, Vector3& aabb_min_out, Vector3& aabb_max_out) const noexcept 
        {
            using Fn = void(*)(const CollisionShape* p_this, float* transform, float* aabb_min_out, float* aabb_max_out);
            reinterpret_cast<Fn*>(m_vtable_ptr)[1](this, transform.Data(), aabb_min_out.Data(), aabb_max_out.Data());
        }

        void GetBoundingSphere(Vector3& center_out, float& radius_out) const noexcept
        {
            using Fn = void(*)(const CollisionShape* p_this, float* center, float* radius);
            reinterpret_cast<Fn*>(m_vtable_ptr)[2](this, center_out.Data(), &radius_out);
        }

        [[nodiscard]] float GetAngularMotionDisc() const noexcept
        { 
            using Fn = float(*)(const CollisionShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[3](this);
        }

        [[nodiscard]] float GetContactBreakingThreshold(float default_threshold) const noexcept
        {
            using Fn = float(*)(const CollisionShape* p_this, float default_threshold);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[4](this, default_threshold);
        }

        void SetLocalScaling(const Vector3& scale) noexcept 
        {
            using Fn = void(*)(CollisionShape* p_this, const Vector3& scale);
            reinterpret_cast<Fn*>(m_vtable_ptr)[5](this, scale);
        }

        [[nodiscard]] const Vector3& GetLocalScaling() const noexcept
        {
            using Fn = const Vector3&(*)(const CollisionShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[6](this);
        }

        void CalculateLocalInertia(float mass, Vector3& inertia_out) const noexcept
        {
            using Fn = void(*)(const CollisionShape* p_this, float mass, float* inertia_out);
            reinterpret_cast<Fn*>(m_vtable_ptr)[7](this, mass, inertia_out.Data());
        }

        [[nodiscard]] const char* GetName() const noexcept 
        { 
            using Fn = const char* (*)(const CollisionShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[8](this); 
        }

        void SetMargin(float margin) noexcept
        {
            using Fn = void(*)(CollisionShape* p_this, float margin);
            reinterpret_cast<Fn*>(m_vtable_ptr)[9](this, margin);
        }

        [[nodiscard]] float GetMargin() const noexcept
        {
            using Fn = float(*)(const CollisionShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[10](this);
        }
    };
    static_assert(offsetof(CollisionShape, m_shape_type) == 0x38);

    struct TriangleCallback
    {
        virtual ~TriangleCallback() noexcept = default;
        virtual void ProcessTriangle(const Vector3* triangle_vertices, int subpart_index, int triangle_index) noexcept = 0;
    };

    struct alignas(16) ConcaveShape : public CollisionShape 
    {
        float m_collision_margin {};

        void ProcessAllTriangles(TriangleCallback* callback, const Vector3& aabb_min, const Vector3& aabb_max) const noexcept
        {
            using Fn = void(*)(const ConcaveShape* p_this, TriangleCallback* callback, const Vector3& aabb_min, const Vector3& aabb_max);
            reinterpret_cast<Fn*>(m_vtable_ptr)[11](this, callback, aabb_min, aabb_max);
        }
    };

    struct alignas(16) ConvexShape : public CollisionShape 
    {
        ////////////////////////////////////
        // Dynamic dispatch methods
        ////////////////////////////////////
        [[nodiscard]] Vector3 LocalGetSupportingVertex(const Vector3& vec) const noexcept
        {
            using Fn = Vector3(*)(const ConvexShape* p_this, const Vector3& vec);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[11](this, vec);
        }

        [[nodiscard]] Vector3 LocalGetSupportingVertexWithoutMargin(const Vector3& vec) const noexcept
        {
            using Fn = Vector3(*)(const ConvexShape* p_this, const Vector3& vec);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[12](this, vec);
        }

        void BatchedUnitVectorGetSupportingVertexWithoutMargin(const Vector3* vectors, Vector3* support_vertices_out, int num_vectors) const noexcept
        {
            using Fn = void(*)(const ConvexShape* p_this, const Vector3* vectors, Vector3* support_vertices_out, int num_vectors);
            reinterpret_cast<Fn*>(m_vtable_ptr)[13](this, vectors, support_vertices_out, num_vectors);
        }

        void GetAabbSlow(const Transform& trans, Vector3& aabbMin, Vector3& aabbMax) const noexcept
        {
            using Fn = void(*)(const ConvexShape* p_this, const Transform& trans, Vector3& min, Vector3& max);
            reinterpret_cast<Fn*>(m_vtable_ptr)[14](this, trans, aabbMin, aabbMax);
        }

        [[nodiscard]] int GetNumPreferredPenetrationDirections() const noexcept
        {
            using Fn = int(*)(const ConvexShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[15](this);
        }

        void GetPreferredPenetrationDirection(int index, Vector3& penetration_vector) const noexcept
        {
            using Fn = void(*)(const ConvexShape* p_this, int index, Vector3& pen);
            reinterpret_cast<Fn*>(m_vtable_ptr)[16](this, index, penetration_vector);
        }
    };

    struct alignas(16) ConvexInternalShape : public ConvexShape 
    {
        Vector3 m_local_scaling {};
        Vector3 m_implicit_shape_dimensions {};
        float   m_collision_margin {};
        float   m_padding {};
    };

    struct alignas(16) PolyhedralConvexShape : public ConvexInternalShape 
    {
    private:
        void* m_polyhedron; //Seems unused or garbage
    public:
        [[nodiscard]] int GetNumVertices() const noexcept
        {
            using Fn = int(*)(const PolyhedralConvexShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[17](this);
        }

        [[nodiscard]] int GetNumEdges() const noexcept
        {
            using Fn = int(*)(const PolyhedralConvexShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[18](this);
        }

        void GetEdge(int i, Vector3& pa, Vector3& pb) const noexcept
        {
            using Fn = void(*)(const PolyhedralConvexShape* p_this, int i, Vector3& pa, Vector3& pb);
            reinterpret_cast<Fn*>(m_vtable_ptr)[19](this, i, pa, pb);
        }

        void GetVertex(int i, Vector3& vtx) const noexcept
        {
            using Fn = void(*)(const PolyhedralConvexShape* p_this, int i, Vector3& vtx);
            reinterpret_cast<Fn*>(m_vtable_ptr)[20](this, i, vtx);
        }

        [[nodiscard]] int GetNumPlanes() const noexcept
        {
            using Fn = int(*)(const PolyhedralConvexShape* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[21](this);
        }

        void GetPlane(Vector3& plane_normal, Vector3& plane_support, int i) const noexcept
        {
            using Fn = int(*)(const PolyhedralConvexShape* p_this, Vector3& plane_normal, Vector3& plane_support, int i);
            reinterpret_cast<Fn*>(m_vtable_ptr)[22](this, plane_normal, plane_support, i);
        }

        [[nodiscard]] bool IsInside(const Vector3& pt, float tolerance) const noexcept
        {
            using Fn = bool(*)(const PolyhedralConvexShape* p_this, const Vector3& pt, float tolerance);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[23](this, pt, tolerance);
        }
    };

    struct alignas(16) BoxShape : public PolyhedralConvexShape // USED IN GAME
    {
        void GetPlaneEquation(Vector4& plane, int i) const noexcept
        {
            using Fn = void(*)(const BoxShape* p_this, Vector4& plane, int i);
            reinterpret_cast<Fn*>(m_vtable_ptr)[24](this, plane, i);
        }
    };

    struct alignas(16) SphereShape : public ConvexInternalShape // USED IN GAME
    {

    };

    struct alignas(16) CapsuleShape : public ConvexInternalShape // USED IN GAME
    {
        int m_up_axis;
    };

    struct alignas(16) CylinderShape : public ConvexInternalShape // USED IN GAME
    {
        int m_up_axis;
    };

    struct alignas(16) StridingMeshInterface
    {
        void**  m_vtable_ptr;
        Vector3 m_scaling {1,1,1};

        void InternalProcessAllTriangles(TriangleCallback* callback, const Vector3& aabbMin, const Vector3& aabbMax) const noexcept
        {
            using Fn = void(*)(const StridingMeshInterface* p_this, TriangleCallback* cb, const Vector3& min, const Vector3& max);
            reinterpret_cast<Fn*>(m_vtable_ptr)[1](this, callback, aabbMin, aabbMax);
        }

        void GetLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, 
                                      int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) noexcept
        {
            using Fn = void(*)(StridingMeshInterface*, unsigned char**, int&, PHY_ScalarType&, int&, unsigned char**, int&, int&, PHY_ScalarType&, int);
            reinterpret_cast<Fn*>(m_vtable_ptr)[2](this, vertexbase, numverts, type, stride, indexbase, indexstride, numfaces, indicestype, subpart);
        }

	    void GetLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, 
                                              int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) const noexcept
        {
            using Fn = void(*)(const StridingMeshInterface*, const unsigned char**, int&, PHY_ScalarType&, int&, const unsigned char**, int&, int&, PHY_ScalarType&, int);
            reinterpret_cast<Fn*>(m_vtable_ptr)[3](this, vertexbase, numverts, type, stride, indexbase, indexstride, numfaces, indicestype, subpart);
        }

        [[nodiscard]] bool HasPremadeAabb() const noexcept
        {
            using Fn = bool(*)(const StridingMeshInterface* p_this);
            return reinterpret_cast<Fn*>(m_vtable_ptr)[9](this);
        }

        void SetPremadeAabb(const Vector3& aabb_min, const Vector3& aabb_max) const noexcept
        {
            using Fn = void(*)(const StridingMeshInterface* p_this, const Vector3& aabb_min, const Vector3& aabb_max);
            reinterpret_cast<Fn*>(m_vtable_ptr)[10](this, aabb_min, aabb_max);
        }

        void GetPremadeAabb(Vector3* aabb_min, Vector3* aabb_max) const noexcept
        {
            using Fn = void(*)(const StridingMeshInterface* p_this, Vector3* aabb_min, Vector3* aabb_max);
            reinterpret_cast<Fn*>(m_vtable_ptr)[11](this, aabb_min, aabb_max);
        }

        // Verifies 4 function offsets vs. base of VTable to verify we're a TriangleVertexMaterialArray
        [[nodiscard]] bool IsInternalTriangleVertexMaterialArray() const noexcept 
        {
            if (!m_vtable_ptr) return false;

            const auto* f0 = reinterpret_cast<const uint8_t*>(m_vtable_ptr[0]);

            const auto check = [&](size_t index, ptrdiff_t expected)
            {
                const auto* fi = reinterpret_cast<const uint8_t*>(m_vtable_ptr[index]);
                return (fi - f0) == expected;
            };

           return check(1,  0x10890) && check(2,  0x11000) && check(3,  0x11000) && check(9, 0x11074) &&
                  check(10, 0x1107C) && check(11, 0x11060) && check(12, 0x1AC);
        }
    };

    struct alignas(16) IndexedMesh
    {
        int m_num_triangles;
        const unsigned char* m_triangle_index_base;
        int m_triangle_index_stride;
        int m_num_vertices;
        const unsigned char* m_vertex_base;
        int m_vertex_stride;
        PHY_ScalarType m_index_type;
        PHY_ScalarType m_vertex_type;
    };

    using IndexedMeshArray = AlignedObjectArray<IndexedMesh>;

    struct alignas(16) TriangleIndexVertexArray : public StridingMeshInterface
    {
        uint8_t          m_unknown_pad[4];
        IndexedMeshArray m_indexed_meshes;
        int              m_pad[2];
        int              m_has_aabb; 
        Vector3          m_aabb_min;
        Vector3          m_aabb_max;
        [[nodiscard]] uint64_t GetAmountTriangles() const noexcept
        {
            if (m_indexed_meshes.m_size < 0 || m_indexed_meshes.m_size > m_indexed_meshes.m_capacity) return 0;
            uint64_t count {};
            for (size_t i{}; i < m_indexed_meshes.m_size; i++)
            {
                IndexedMesh& mesh = m_indexed_meshes.m_data[i];
                count += mesh.m_num_triangles;
            }
            return count;
        }
    };
    static_assert(offsetof(TriangleIndexVertexArray, m_indexed_meshes) == 36);
    static_assert(offsetof(TriangleIndexVertexArray, m_aabb_min) == 80);

    struct alignas(16) MaterialProperties
    {
        int m_num_materials;
        const unsigned char* m_material_base;
        int m_material_stride;
        PHY_ScalarType m_material_type;
        int m_num_triangles;
        const unsigned char* m_triangle_materials_base;
        int m_triangle_material_stride;
        PHY_ScalarType m_triangle_type;
    };
    static_assert(sizeof(MaterialProperties) == 48);

    struct alignas(16) TriangleIndexVertexMaterialArray : public TriangleIndexVertexArray // USED IN GAME with MultimaterialTriangleMeshShape
    {
        using MaterialArray = AlignedObjectArray<MaterialProperties>;
        MaterialArray m_materials;

        void GetLockedMaterialBase(unsigned char** material_base, int& num_materials, PHY_ScalarType& material_type, int& material_stride,
								unsigned char** triangle_materialBase, int& num_triangles, int& triangle_material_stride, PHY_ScalarType& triangle_type, int subpart = 0) noexcept
        {
            using Fn = void(*)(TriangleIndexVertexMaterialArray*, unsigned char**, int&, PHY_ScalarType&, int&, unsigned char**, int&, int&, PHY_ScalarType&, int);
            reinterpret_cast<Fn*>(m_vtable_ptr)[12](this, material_base, num_materials, material_type, material_stride, triangle_materialBase, num_triangles, triangle_material_stride, triangle_type, subpart);
        }

	    void GetLockedReadOnlyMaterialBase(const unsigned char** material_base, int& num_materials, PHY_ScalarType& material_type, int& material_stride,
										 const unsigned char** triangle_materialBase, int& num_triangles, int& triangle_material_stride, PHY_ScalarType& triangle_type, int subpart = 0) const noexcept
        {
            using Fn = void(*)(const TriangleIndexVertexMaterialArray*, const unsigned char**, int&, PHY_ScalarType&, int&, const unsigned char**, int&, int&, PHY_ScalarType&, int);                                   
            reinterpret_cast<Fn*>(m_vtable_ptr)[13](this, material_base, num_materials, material_type, material_stride, triangle_materialBase, num_triangles, triangle_material_stride, triangle_type, subpart);
        }
    };
    static_assert(offsetof(TriangleIndexVertexMaterialArray, m_materials) == 112);

    struct alignas(16) TriangleMeshShape : public ConcaveShape
    {
        Vector3 m_local_aabb_min {};
        Vector3 m_local_aabb_max {};
        StridingMeshInterface* m_mesh_interface {};
    };

    struct alignas(16) QuantizedBvhNode
    {
        unsigned short int m_quantized_aabb_min[3];
        unsigned short int m_quantized_aabb_max[3];
        int m_escape_index_or_triangle_index;
    };

    struct alignas(16) OptimizedBvhNode
    {
        Vector3 m_aabb_min_org;
        Vector3 m_aabb_max_org;
        int m_escape_index;
        int m_sub_part;
        int m_triangle_index;
        char m_padding[20];
    };

    struct alignas(16) BvhSubtreeInfo
    {
        unsigned short int m_quantized_aabb_min[3];
        unsigned short int m_quantized_aabb_max[3];
        int m_root_node_index;
        int m_subtree_size;
        int m_padding[3];
    };

    struct alignas(16) QuantizedBvh
    {
        using NodeArray           = AlignedObjectArray<OptimizedBvhNode>;
        using QuantizedNodeArray  = AlignedObjectArray<QuantizedBvhNode>;
        using BvhSubtreeInfoArray = AlignedObjectArray<BvhSubtreeInfo>;
        enum TraversalMode { TRAVERSAL_STACKLESS = 0, TRAVERSAL_STACKLESS_CACHE_FRIENDLY, TRAVERSAL_RECURSIVE};
        void**  m_vtable_ptr;
        Vector3 m_bvh_aabb_min;
        Vector3 m_bvh_aabb_max;
        Vector3 m_bvh_quantization;
        int m_bullet_version;
        int m_cur_node_index;
        bool m_use_quantization;
        NodeArray m_leaf_nodes;
        NodeArray m_contiguous_nodes;
        QuantizedNodeArray m_quantized_leaf_nodes;
        QuantizedNodeArray m_quantized_contiguous_nodes;
        TraversalMode m_traversal_mode;
        BvhSubtreeInfoArray m_subtree_headers;
        int m_subtree_header_count;
    };

    template <class Key, class Value>
    struct HashMap
    {
        AlignedObjectArray<int>   m_hash_table;
        AlignedObjectArray<int>   m_next;
        AlignedObjectArray<Value> m_value_array;
        AlignedObjectArray<Key>   m_key_array;
    };

    struct TriangleInfo
    {
        int   m_flags;
        float m_edge_V0V1_angle;
        float m_edge_V1V2_angle;
        float m_edge_V2V0_angle;
    };

    using InternalTriangleInfoMap = HashMap<int, TriangleInfo>;

    struct TriangleInfoMap : public InternalTriangleInfoMap
    {
        float m_convex_epsilon;
        float m_planar_epsilon;
        float m_equal_vertex_threshold;
        float m_edge_distance_threshold;
        float m_max_edge_angle_threshold; 
        float m_zero_area_threshold; 
    };

    struct alignas(16) OptimizedBvh : public QuantizedBvh {};

    struct alignas(16) BvhTriangleMeshShape : public TriangleMeshShape
    {
        OptimizedBvh* m_bvh {};
        TriangleInfoMap* m_triangle_info_map {};
        bool m_use_quantized_aabb_compression {};
        bool m_owns_bvh {};
        bool m_pad[11] {};
    };

    struct alignas(16) ScaledBvhTriangleMeshShape : public ConcaveShape // USED IN GAME
    {
        Vector3 m_local_scaling {};
        BvhTriangleMeshShape* m_bvh_tri_mesh_shape {};
    };

    struct Material
    {
        float m_friction;
        float m_restitution;
        int   m_int_flags_1;
        int   m_int_flags_2;
    };

    struct alignas(16) MultimaterialTriangleMeshShape : public BvhTriangleMeshShape // USED IN GAME
    {
	    AlignedObjectArray<Material*> m_material_list;
        [[nodiscard]] uint64_t GetAmountTriangles() const noexcept
        {
            if (m_mesh_interface->IsInternalTriangleVertexMaterialArray())
            {
                return reinterpret_cast<TriangleIndexVertexMaterialArray*>(m_mesh_interface)->GetAmountTriangles();
            }
            return 0;
        }
    };
    static_assert(offsetof(MultimaterialTriangleMeshShape, m_local_aabb_min) ==  0x60);

    struct DbvtAabbMm
    {
        Vector3 m_min, m_max;
    };

    struct DbvtNode
    {
        using DbvtVolume = DbvtAabbMm;
        DbvtVolume m_volume;
        DbvtNode* m_parent;
        union 
        {
            DbvtNode* m_children[2];
            void* m_data;
            int m_data_as_int;
        };
        [[nodiscard]] bool IsLeaf() const { return (m_children[1] == 0); }
        [[nodiscard]] bool IsInternal() const { return (!IsLeaf()); }
    };

    struct alignas(16) CompoundShapeChild
    {
        Transform m_transform;
        CollisionShape* m_child_shape;
        int m_child_shape_type;
        float m_child_margin;
        DbvtNode* m_node;
    };
    static_assert(sizeof(CompoundShapeChild) == 96);

    struct Dbvt 
    {
        struct sStkNN
        {
            const DbvtNode* a;
            const DbvtNode* b;
        };
        DbvtNode* m_root;
        DbvtNode* m_free;
        int m_lkhd;
        int m_leaves;
        unsigned m_opath;
        AlignedObjectArray<sStkNN> m_stk_stack;
    };

    struct alignas(16) CompoundShape : public CollisionShape // USED IN GAME
    {
        uint8_t m_padd[4];
        AlignedObjectArray<CompoundShapeChild> m_children;
        Vector3 m_local_aabb_min;
        Vector3 m_local_aabb_max;
        Dbvt* m_dynamic_aabb_tree;
        int m_update_revision;
        float m_collision_margin;
        Vector3 m_local_scaling;

        void RemoveChildShape(CollisionShape* shape) noexcept
        {
            using Fn = void(*)(CompoundShape* p_this, CollisionShape* shape);
            reinterpret_cast<Fn*>(m_vtable_ptr)[11](this, shape);
        }

        void RecalculateLocalAabb() noexcept
        {
            using Fn = void(*)(CompoundShape* p_this);
            reinterpret_cast<Fn*>(m_vtable_ptr)[12](this);
        }
    };
    static_assert(offsetof(CompoundShape, m_children) == 84);

    template <typename TTarget>
    requires std::is_base_of<CollisionShape, TTarget>::value
    [[nodiscard]] constexpr inline bool IsShapeType(const CollisionShape* shape) noexcept
    {
        using CleanTarget = std::remove_cv_t<TTarget>;
        if (!shape) return false;
        else if constexpr (std::is_same_v<CleanTarget, BoxShape>) return shape->m_shape_type == BroadphaseNativeTypes::BOX_SHAPE_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, SphereShape>) return shape->m_shape_type == BroadphaseNativeTypes::SPHERE_SHAPE_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, CapsuleShape>) return shape->m_shape_type == BroadphaseNativeTypes::CAPSULE_SHAPE_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, ScaledBvhTriangleMeshShape>) return shape->m_shape_type == BroadphaseNativeTypes::SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, MultimaterialTriangleMeshShape>) return shape->m_shape_type == BroadphaseNativeTypes::MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, CompoundShape>) return shape->m_shape_type == BroadphaseNativeTypes::COMPOUND_SHAPE_PROXYTYPE;
        else if constexpr (std::is_same_v<CleanTarget, CylinderShape>) return shape->m_shape_type == BroadphaseNativeTypes::CYLINDER_SHAPE_PROXYTYPE;
        else return false;
    }

    template <typename TTarget>
    requires std::is_base_of<CollisionShape, TTarget>::value
    [[nodiscard]] constexpr inline TTarget* SafeShapeCast(const CollisionShape* shape) noexcept
    {
        if (IsShapeType<TTarget>(shape)) return reinterpret_cast<TTarget*>(shape);
        else return nullptr;
    }

    struct MotionState
    {
        void** m_vtable_ptr = nullptr;

        void GetWorldTransform(Transform& world_trans) const 
        {
            using Fn = void(*)(const MotionState* p_this, Transform& world_trans);
            reinterpret_cast<Fn*>(m_vtable_ptr)[1](this, world_trans);
        }

        void SetWorldTransform(const Transform& world_trans) 
        {
            using Fn = void(*)(MotionState* p_this, const Transform& world_trans);
            reinterpret_cast<Fn*>(m_vtable_ptr)[2](this, world_trans);
        }
    };

    //fwd
    struct BroadphaseProxy;
    struct alignas(16) CollisionObject
    {
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
        void**              m_vtable_ptr;                  
        uint8_t             m_header_padding[24];          
        Transform           m_transform_matrix;             
        Transform           m_interpolation_world_transform;
        Vector3             m_interpolation_linear_velocity; 
        Vector3             m_interpolation_angular_velocity;
        Vector3             m_anisotropic_friction;                 
        int32_t             m_has_anisotropic_friction;      
        float               m_contact_processing_threshold;  
        BroadphaseProxy*    m_broadphase_proxy_ptr;          
        CollisionShape*     m_collision_shape_ptr;           
        void*               m_extension_pointer;             
        CollisionShape*     m_root_collision_shape_ptr;         
        int                 m_collision_flags;               
        int                 m_island_tag_1;                  
        int                 m_companion_id;                   
        int                 m_world_array_index;              
        int                 m_activation_state_1;             
        float               m_friction;                       
        float               m_restitution;                    
        int                 m_internal_type;                  
        void*               m_user_object_pointer;                        
        float               m_hit_fraction;                   
        float               m_ccd_swept_sphere_radius;        
        float               m_ccd_motion_threshold;           
        int                 m_check_collide_with;   
        uint8_t             m_padding_align[16];                        
        AlignedObjectArray<const CollisionObject*> m_objects_without_collision_check; 
        int                 m_update_revision; 

        // Constructor: __int64 __fastcall sub_1463578F8(__int64 a1)

        void SetCollisionShape(CollisionShape* shape) noexcept
        {
            using Fn = void(*)(CollisionObject* p_this, CollisionShape* shape);
            reinterpret_cast<Fn*>(m_vtable_ptr)[2](this, shape);
        }
        
        [[nodiscard]] bool IsRigidBody() noexcept
        {
            return m_internal_type == CO_RIGID_BODY;
        }

        [[nodiscard]] bool IsGhostObject() noexcept
        {
            return m_internal_type == CO_GHOST_OBJECT;
        }
    };
    static_assert(offsetof(CollisionObject, m_transform_matrix)     == 0x20);
    static_assert(offsetof(CollisionObject, m_broadphase_proxy_ptr) == 0xD8);
    static_assert(offsetof(CollisionObject, m_collision_shape_ptr)  == 0xE0);
    static_assert(offsetof(CollisionObject, m_internal_type)        == 0x114);

    struct alignas(16) RigidBodyConstructionInfo
    {
        float               m_mass;
        uint8_t             m_pad_0[4];
        MotionState*        m_motion_state;
        Transform           m_start_world_transform;
        CollisionShape*     m_collision_shape;
        uint8_t             m_pad_1[8];
        Vector3             m_local_inertia;
        float               m_linear_sleeping_threshold;
        float               m_angular_sleeping_threshold;
        float               m_friction;
        float               m_restitution;
        float               m_linear_damping;
        float               m_angular_damping;
        bool                m_additional_damping;
        uint8_t             m_pad_2[3];
        float               m_additional_damping_factor;
        float               m_additional_linear_damping_threshold_sqr;
        float               m_additional_angular_damping_threshold_sqr;
        float               m_additional_angular_damping_factor;
    };
    static_assert(sizeof(RigidBodyConstructionInfo) == 160);

    struct alignas(16) RigidBody : public CollisionObject
    {
        Vector3             m_linear_velocity;            
        Vector3             m_angular_velocity;           
        float               m_inverse_mass;               
        uint8_t             m_padding_inv_mass[12]; 
        Vector3             m_linear_factor; 
        Vector3             m_gravity_acceleration;  
        Vector3             m_gravity;   
        Vector3             m_inv_inertia_local; 
        Vector3             m_total_force;
        Vector3             m_total_torque;
        float               m_linear_damping;                          
        float               m_angular_damping;                         
        float               m_linear_sleeping_threshold;               
        float               m_angular_sleeping_threshold;              
        float               m_additional_damping_factor;               
        float               m_additional_linear_damping_threshold_sqr; 
        float               m_additional_angular_damping_threshold_sqr;
        float               m_additional_angular_damping_factor;       
        float               m_additional_damping_unknown;              
        float               m_additional_damping_active;
        MotionState*        m_optional_motion_state;
        AlignedObjectArray<void*> m_constraint_refs;
        int                 m_rigid_body_flags;
        int                 m_debug_body_id;
        uint8_t             m_padding_debug_id[12];
        Matrix3x3           m_inv_inertia_tensor_world;
        
        Vector3             m_delta_linear_velocity;  
        Vector3             m_delta_angular_velocity;  
        Vector3             m_angular_factor;     
        Vector3             m_inv_mass_vector; 
        Vector3             m_push_velocity;
        Vector3             m_turn_velocity;
        
        int32_t             m_unknown_padding_736;
        int32_t             m_unknown_padding_740;              

        // SetupRigidBody(const RigidBodyConstructionInfo& construction_info) at: base + 0x63539E0
        // Rigidbody constructors:
        // _int64 __fastcall sub_1463525A8(__int64 a1, __int64 a2)
        // __int64 __fastcall sub_1463525F8(__int64 a1, float a2, __int64 a3, __int64 a4, __int128 *a5)
    };
    static_assert(offsetof(RigidBody, m_inverse_mass)   == 0x180);
    static_assert(offsetof(RigidBody, m_gravity)        == 0x1B0);
    static_assert(offsetof(RigidBody, m_angular_factor) == 0x2A0);
    static_assert(offsetof(RigidBody, m_turn_velocity)  == 0x2D0);

    struct alignas(16) GhostObject : public CollisionObject
    {
        AlignedObjectArray<CollisionObject*> m_overlapping_objects;
        // Constructor at: base + 0x63661F0
    };

    struct alignas(16) BroadphaseProxy
    {
        CollisionObject*  m_client_body {};
        int               m_collision_filter_group;
        int               m_collision_filter_mask;
        int               m_unique_id;
        Vector3           m_aabb_min;
        Vector3           m_aabb_max;
    };
    static_assert(offsetof(BroadphaseProxy, m_aabb_min) == 0x20);
    static_assert(offsetof(BroadphaseProxy, m_aabb_max) == 0x30);

    struct DispatcherInfo 
    {
        float   m_time_step;
        int     m_step_count;  
        int     m_dispatch_func;
        float   m_time_of_impact;
        bool    m_use_continuous;
        uint8_t m_pad_11[7];
        void*   m_debug_draw;
        uint8_t m_pad_20[40];
    };
    static_assert(sizeof(DispatcherInfo) == 0x48);

    struct ContactSolverInfo
    {
        float   m_tau;
        float   m_damping;
        float   m_friction;
        float   m_time_step;
        uint8_t m_pad[80];
    };

    struct CollisionWorld
    {
        void** m_vtable_ptr;
        uint32_t m_unknown_pad_08;            
        AlignedObjectArray<GhostObject*> m_ghost_objects;
        uint32_t m_unknown_pad_28;       
        AlignedObjectArray<RigidBody*> m_rigid_bodies;
        void* m_dispatcher1; 
        DispatcherInfo m_dispatch_info;   
        void* m_broadphase_pair_cache; 
        void* m_debug_drawer;     
        bool m_force_update_all_aabbs;  
        uint8_t m_pad_A9[7];   

        // updateSingleAabb(CollisionObject* colObj): Asphalt9_Steam_x64_rtl.exe+6365A4C 
        // updateAabbs():                             Asphalt9_Steam_x64_rtl.exe+63659B0
        // performDiscreteCollisionDetection():       Asphalt9_Steam_x64_rtl.exe+636405C
    };
    static_assert(offsetof(CollisionWorld, m_ghost_objects)              == 0x0C);
    static_assert(offsetof(CollisionWorld, m_ghost_objects.m_data)       == 0x18);
    static_assert(offsetof(CollisionWorld, m_dispatcher1)                == 0x48);
    static_assert(offsetof(CollisionWorld, m_rigid_bodies.m_data)        == 0x38);
    static_assert(offsetof(CollisionWorld, m_dispatch_info)              == 0x50);
    static_assert(offsetof(CollisionWorld, m_broadphase_pair_cache)      == 0x98);
    static_assert(offsetof(CollisionWorld, m_debug_drawer)               == 0xA0);
    static_assert(offsetof(CollisionWorld, m_force_update_all_aabbs)     == 0xA8);
    static_assert(sizeof(CollisionWorld)                                 == 0xB0);

    struct DynamicsWorld : public CollisionWorld
    {
        void* m_internal_tick_callback; 
        void* m_internal_pre_tick_callback;
        void* m_world_user_info;
        ContactSolverInfo m_solver_info;
    };
    static_assert(offsetof(DynamicsWorld, m_internal_tick_callback)     == 0xB0);
    static_assert(offsetof(DynamicsWorld, m_internal_pre_tick_callback) == 0xB8);
    static_assert(offsetof(DynamicsWorld, m_solver_info)                == 0xC8);
    static_assert(offsetof(DynamicsWorld, m_solver_info.m_time_step)    == 0xD4);
 
    struct alignas(16) DiscreteDynamicsWorld : public DynamicsWorld
    {
        // internalSingleStepSimulation(btScalar timeStep): Asphalt9_Steam_x64_rtl.exe+634C7CC
    };

    [[nodiscard]] inline BulletTypes::UnalignedTransform ComposeBulletTransforms(const BulletTypes::UnalignedTransform& parent, const BulletTypes::UnalignedTransform& child) noexcept
    {
        BulletTypes::UnalignedTransform result{};

        const auto Dot3 = [](const BulletTypes::UnalignedVector4& a, const BulletTypes::UnalignedVector4& b) -> float
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        };

        for (int col = 0; col < 3; ++col)
        {
            auto& out = result[col];

            for (int row = 0; row < 3; ++row)
            {
                BulletTypes::UnalignedVector4 parent_row
                {
                    parent[0].x, parent[1].x, parent[2].x, 0.0f
                };

                if (row == 1)
                    parent_row = { parent[0].y, parent[1].y, parent[2].y, 0.0f };
                else if (row == 2)
                    parent_row = { parent[0].z, parent[1].z, parent[2].z, 0.0f };

                const auto& childCol = child[col];

                float value = Dot3(parent_row, childCol);

                if (row == 0) out.x = value;
                else if (row == 1) out.y = value;
                else out.z = value;
            }

            out.w = 0.0f;
        }

        const auto& childPos = child[3];
        const auto& parentPos = parent[3];

        result[3].x = parent[0].x * childPos.x + parent[1].x * childPos.y + parent[2].x * childPos.z + parentPos.x;
        result[3].y = parent[0].y * childPos.x + parent[1].y * childPos.y + parent[2].y * childPos.z + parentPos.y;
        result[3].z = parent[0].z * childPos.x + parent[1].z * childPos.y + parent[2].z * childPos.z + parentPos.z;
        result[3].w = 1.0f;

        return result;
    }

}