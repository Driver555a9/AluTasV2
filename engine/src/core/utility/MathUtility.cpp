#include "core/utility/MathUtility.h"

#include "core/utility/Assert.h"

namespace CoreEngine
{
    MathUtility::Raytest3D_Result MathUtility::RaySphereIntersect(const Ray3D& ray, const Sphere& sphere) noexcept
    {
        const glm::vec3 offset = ray.m_origin - sphere.m_center;

        const float projection   = glm::dot(offset, ray.m_direction_normalized);
        const float discriminant = projection * projection - (glm::dot(offset, offset) - sphere.m_radius * sphere.m_radius);

        if (discriminant < 0.0f)
            return Raytest3D_Result{};

        const float sqrt_discriminant = std::sqrt(discriminant);

        const float t0 = -projection - sqrt_discriminant;
        const float t1 = -projection + sqrt_discriminant;

        const float t_hit = (t0 >= 0.0f) ? t0 : ((t1 >= 0.0f) ? t1 : -1.0f);

        if (t_hit < 0.0f)
            return Raytest3D_Result{};

        const glm::vec3 hit_point = ray.m_origin + t_hit * ray.m_direction_normalized;

        return Raytest3D_Result{true, hit_point, 0.0f}; 
    }

    MathUtility::Raytest3D_Result MathUtility::RayAABBIntersect(const Ray3D& ray, const AABB& aabb) noexcept
    {
        float t_min = std::numeric_limits<float>::lowest();
        float t_max = std::numeric_limits<float>::max();

        const glm::vec3 min_corner = aabb.min();
        const glm::vec3 max_corner = aabb.max();

        for (int axis = 0; axis < 3; ++axis)
        {
            const float ray_origin_axis     = ray.m_origin[axis];
            const float ray_direction_axis  = ray.m_direction_normalized[axis];
            const float box_min_axis        = min_corner[axis];
            const float box_max_axis        = max_corner[axis];

            if (std::abs(ray_direction_axis) < 1e-8f)
            {
                if (ray_origin_axis < box_min_axis || ray_origin_axis > box_max_axis)
                {
                    return Raytest3D_Result { false };
                }
            }
            else
            {
                const float inv_dir = 1.0f / ray_direction_axis;
                float t1 = (box_min_axis - ray_origin_axis) * inv_dir;
                float t2 = (box_max_axis - ray_origin_axis) * inv_dir;

                if (t1 > t2) std::swap(t1, t2);

                t_min = std::max(t_min, t1);
                t_max = std::min(t_max, t2);

                if (t_min > t_max)
                    return Raytest3D_Result { false };
            }
        }

        const float t = (t_min >= 0.0f) ? t_min : t_max;

        if (t < 0.0f) { return Raytest3D_Result{ false }; }

        const glm::vec3 hit_point = ray.m_origin + t * ray.m_direction_normalized;

        return Raytest3D_Result{true, hit_point, 0.0f};
    }

    MathUtility::Raytest3D_Result MathUtility::RayTriangleIntersect(const Ray3D& ray, const Triangle& triangle) noexcept
    {
        const glm::vec3 edge1 = triangle.m_vertex_1 - triangle.m_vertex_0;
        const glm::vec3 edge2 = triangle.m_vertex_2 - triangle.m_vertex_0;

        const glm::vec3 pvec = glm::cross(ray.m_direction_normalized, edge2);
        const float det = glm::dot(edge1, pvec);

        if (std::abs(det) < 1e-8f)
            return Raytest3D_Result{};

        const float inv_det = 1.0f / det;

        const glm::vec3 tvec = ray.m_origin - triangle.m_vertex_0;

        const float u = glm::dot(tvec, pvec) * inv_det;
        if (u < 0.0f || u > 1.0f)
            return Raytest3D_Result{ false };

        const glm::vec3 qvec = glm::cross(tvec, edge1);
        const float v = glm::dot(ray.m_direction_normalized, qvec) * inv_det;

        if (v < 0.0f || u + v > 1.0f)
            return Raytest3D_Result{ false };

        const float t = glm::dot(edge2, qvec) * inv_det;

        if (t < 1e-6f)
            return Raytest3D_Result{ false };

        const glm::vec3 hit_point = ray.m_origin + t * ray.m_direction_normalized;

        return Raytest3D_Result{true, hit_point, det};
    }

    MathUtility::Raytest3D_Result MathUtility::RayPlaneIntersect(const Ray3D& ray, const Plane& plane) noexcept
    {
        const float denom = glm::dot(plane.m_normal_normalized, ray.m_direction_normalized);

        if (std::abs(denom) < 1e-8f)
            return Raytest3D_Result{ false };

        const float t = glm::dot(plane.m_point - ray.m_origin, plane.m_normal_normalized) / denom;

        if (t < 1e-6f) 
            return Raytest3D_Result{ false };

        const glm::vec3 hit_point = ray.m_origin + t * ray.m_direction_normalized;

        return Raytest3D_Result{true, hit_point, 0.0f};
    }

    bool MathUtility::SphereIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const Sphere& sphere) noexcept
    {
        for (auto p : planes)
        {
            const float length = glm::length(glm::vec3(p));
            p /= length;

            if (glm::dot(glm::vec3(p), sphere.m_center) + p.w < -sphere.m_radius)
                return false;
        }
        return true;
    }
    
    bool MathUtility::AABBIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const AABB& aabb) noexcept
    {
        for (auto p : planes)
        {
            const float length = glm::length(glm::vec3(p));
            p /= length;

            const glm::vec3 normal = glm::vec3(p);

            const float distance = glm::dot(normal, aabb.m_center) + p.w;

            const float r = glm::dot(glm::abs(normal), aabb.m_half_extents);

            if (distance < -r)
                return false;
        }

        return true;
    }

    bool MathUtility::LineIsInFrustum(const ViewProjectionPlanes_ReverseZ& planes, const Line& line) noexcept
    {
        for (auto p : planes)
        {
            const float len = glm::length(glm::vec3(p));
            p /= len;

            const float dA = glm::dot(glm::vec3(p), line.m_point_a) + p.w;
            const float dB = glm::dot(glm::vec3(p), line.m_point_b) + p.w;

            if (dA < 0.0f && dB < 0.0f)
                return false;
        }

        return true;
    }

    MathUtility::ViewProjectionPlanes_ReverseZ MathUtility::ExtractProjectionPlanesFromVP(const glm::mat4& vp) noexcept
    {
        const glm::vec4 row1 = { vp[0][0], vp[1][0], vp[2][0], vp[3][0] };
        const glm::vec4 row2 = { vp[0][1], vp[1][1], vp[2][1], vp[3][1] };
        const glm::vec4 row3 = { vp[0][2], vp[1][2], vp[2][2], vp[3][2] };
        const glm::vec4 row4 = { vp[0][3], vp[1][3], vp[2][3], vp[3][3] };

        return std::array<glm::vec4, 5>
        {
            row4 + row1, // Left
            row4 - row1, // Right
            row4 + row2, // Bottom
            row4 - row2, // Top
            row4 + row3  // Near
            // no far plane
        };
    }
}