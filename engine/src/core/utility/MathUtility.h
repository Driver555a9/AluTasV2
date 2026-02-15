#pragma once

#include "glm/glm.hpp"

#include <array>

namespace CoreEngine
{
    namespace MathUtility
    {
    //////////////////////////////////////////////// 
    //---------  Raycasting
    //////////////////////////////////////////////// 
        struct Raytest3D_Result 
        {
            bool  m_has_hit {false};
            glm::vec3 m_intersection_point;
            float m_determinant {0.0f};
        };

        struct Ray3D 
        {
            glm::vec3 m_origin {}; 
            glm::vec3 m_direction_normalized {};
            float     m_max_ray_length {1000.0f};
        };

        struct Plane
        {
            glm::vec3 m_point;
            glm::vec3 m_normal_normalized;
            float     m_plane_distance {};
        };

        struct Sphere
        {
            glm::vec3 m_center;
            float     m_radius;
        };

        struct Triangle
        {
            glm::vec3 m_vertex_0;
            glm::vec3 m_vertex_1;
            glm::vec3 m_vertex_2;
        };

        struct Line
        {
            glm::vec3 m_point_a;
            glm::vec3 m_point_b;
        };

        struct AABB
        {
            glm::vec3 m_half_extents;
            glm::vec3 m_center;

            [[nodiscard]] constexpr inline glm::vec3 min() const { return m_center - m_half_extents; }
            [[nodiscard]] constexpr inline glm::vec3 max() const { return m_center + m_half_extents; }

            constexpr AABB() noexcept = default;
            
            constexpr AABB(const glm::vec3& local_half_extents, const glm::vec3& local_center) noexcept : m_half_extents(local_half_extents), m_center(local_center) {}

            [[nodiscard]] constexpr inline static AABB CreateWorldSpaceAABB(const glm::mat4& model_mat, const glm::vec3& local_half_extents, const glm::vec3& local_center) noexcept
            {
                AABB out;
                const glm::mat3 absMatrix = glm::mat3( glm::abs(glm::vec3(model_mat[0])), glm::abs(glm::vec3(model_mat[1])), glm::abs(glm::vec3(model_mat[2])));
                out.m_half_extents = absMatrix * (local_half_extents);
                out.m_center = glm::vec3(model_mat * glm::vec4(local_center, 1.0f));
                return out;
            }
        };

        using ViewProjectionPlanes_ReverseZ = std::array<glm::vec4, 5>;

        [[nodiscard]] Raytest3D_Result RaySphereIntersect(const Ray3D& ray, const Sphere& sphere) noexcept;

        [[nodiscard]] Raytest3D_Result RayAABBIntersect(const Ray3D& ray, const AABB& aabb) noexcept; 

        [[nodiscard]] Raytest3D_Result RayTriangleIntersect(const Ray3D& ray, const Triangle& triangle) noexcept;

        [[nodiscard]] Raytest3D_Result RayPlaneIntersect(const Ray3D& ray, const Plane& plane) noexcept;

        [[nodiscard]] bool SphereIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const Sphere& sphere) noexcept;

        [[nodiscard]] bool AABBIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const AABB& aabb) noexcept;

        [[nodiscard]] bool LineIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const Line& line) noexcept;

         [[nodiscard]] ViewProjectionPlanes_ReverseZ ExtractProjectionPlanesFromVP(const glm::mat4& vp) noexcept;
    }

}