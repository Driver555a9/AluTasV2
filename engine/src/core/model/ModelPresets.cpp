#include "core/model/ModelPresets.h"

namespace CoreEngine
{
    SimpleSphere_radius1::SimpleSphere_radius1(float radius, int subdivisions)
    : m_sphere_verts(SimpleCube_1x1x1::CUBE_VERTICES), m_sphere_indices(SimpleCube_1x1x1::CUBE_INDICES)
    {
        for (size_t s = 0; s < static_cast<size_t>(subdivisions); ++s)
        {
            std::vector<Vertex> newVerts;
            newVerts.reserve(m_sphere_indices.size() * 6);
            std::vector<GLuint> newIndices;
            newIndices.reserve(m_sphere_indices.size() * 12);

            for (size_t i = 0; i < m_sphere_indices.size(); i += 3)
            {
                const glm::vec3 v0 = m_sphere_verts[m_sphere_indices[i + 0]].m_position;
                const glm::vec3 v1 = m_sphere_verts[m_sphere_indices[i + 1]].m_position;
                const glm::vec3 v2 = m_sphere_verts[m_sphere_indices[i + 2]].m_position;

                const glm::vec3 m0 = glm::normalize((v0 + v1) * 0.5f) * radius;
                const glm::vec3 m1 = glm::normalize((v1 + v2) * 0.5f) * radius;
                const glm::vec3 m2 = glm::normalize((v2 + v0) * 0.5f) * radius;

                const GLuint idx = newVerts.size();

                newVerts.emplace_back( glm::normalize(v0) * radius, glm::normalize(v0), glm::vec2(0.0f) );
                newVerts.emplace_back( glm::normalize(v1) * radius, glm::normalize(v1), glm::vec2(0.0f) );
                newVerts.emplace_back( glm::normalize(v2) * radius, glm::normalize(v2), glm::vec2(0.0f) );
                newVerts.emplace_back( m0, glm::normalize(m0), glm::vec2(0.0f) );
                newVerts.emplace_back( m1, glm::normalize(m1), glm::vec2(0.0f) );
                newVerts.emplace_back( m2, glm::normalize(m2), glm::vec2(0.0f) );

                const auto AddTriangle = [&](GLuint i1, GLuint i2, GLuint i3)
                {
                    const glm::vec3 a = newVerts[i1].m_position;
                    const glm::vec3 b = newVerts[i2].m_position;
                    const glm::vec3 c = newVerts[i3].m_position;

                    const glm::vec3 normal = glm::cross(b - a, c - a);

                    if (glm::dot(normal, a) < 0.0f)
                        std::swap(i2, i3);

                    newIndices.push_back(i1);
                    newIndices.push_back(i2);
                    newIndices.push_back(i3);
                };

                AddTriangle(idx + 0, idx + 3, idx + 5);
                AddTriangle(idx + 3, idx + 1, idx + 4);
                AddTriangle(idx + 5, idx + 4, idx + 2);
                AddTriangle(idx + 3, idx + 4, idx + 5);
            }

            m_sphere_verts   = std::move(newVerts);
            m_sphere_indices = std::move(newIndices);
        }
    }

    SimpleCylinder_r1_h2::SimpleCylinder_r1_h2(int segments) noexcept
    {
        const float radius     = 1.0f;
        const float half_height = 1.0f;

        m_verts.push_back({ glm::vec3(0, half_height, 0), glm::vec3(0, 1, 0), glm::vec2(0.5f, 0.5f) });
        m_verts.push_back({ glm::vec3(0, -half_height, 0), glm::vec3(0, -1, 0), glm::vec2(0.5f, 0.5f) });

        int top_center_index = 0;
        int bottom_center_index = 1;
        int top_ring_start = m_verts.size();

        for (int i = 0; i <= segments; ++i)
        {
            float u = static_cast<float>(i) / segments;
            float theta = u * glm::two_pi<float>();
            float cos_theta = std::cos(theta);
            float sin_theta = std::sin(theta);

            float x = cos_theta * radius;
            float z = sin_theta * radius;

            m_verts.push_back({ glm::vec3(x, half_height, z), glm::vec3(0, 1, 0), glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f) });
            m_verts.push_back({ glm::vec3(x, -half_height, z), glm::vec3(0, -1, 0), glm::vec2(x * 0.5f + 0.5f, z * 0.5f + 0.5f) });
            m_verts.push_back({ glm::vec3(x, half_height, z), glm::vec3(cos_theta, 0, sin_theta), glm::vec2(u, 1.0f) });
            m_verts.push_back({ glm::vec3(x, -half_height, z), glm::vec3(cos_theta, 0, sin_theta), glm::vec2(u, 0.0f) });
        }

        for (int i = 0; i < segments; ++i)
        {
            int baseIdx = top_ring_start + (i * 4);
            int nextBaseIdx = top_ring_start + ((i + 1) * 4);

            m_indices.push_back(top_center_index);
            m_indices.push_back(baseIdx + 0);
            m_indices.push_back(nextBaseIdx + 0);

            m_indices.push_back(bottom_center_index);
            m_indices.push_back(nextBaseIdx + 1);
            m_indices.push_back(baseIdx + 1);

            m_indices.push_back(baseIdx + 2);
            m_indices.push_back(baseIdx + 3);
            m_indices.push_back(nextBaseIdx + 2);

            m_indices.push_back(nextBaseIdx + 2);
            m_indices.push_back(baseIdx + 3);
            m_indices.push_back(nextBaseIdx + 3);
        }
    }

    SimpleCapsule_r1_h2::SimpleCapsule_r1_h2(int segments, int rings) noexcept
    {
        const float radius = 1.0f;
        const float cylinder_half_height = 1.0f; 
        
        for (int i = 0; i <= rings * 2 + 1; ++i)
        {
            bool isTopHemisphere = (i <= rings);
            int ringIndex = isTopHemisphere ? i : i - 1;

            float v = static_cast<float>(ringIndex) / (rings * 2); 
            float phi = glm::half_pi<float>() - v * glm::pi<float>(); 
            
            float yOffset = isTopHemisphere ? cylinder_half_height : -cylinder_half_height;
            
            float y = std::sin(phi) * radius;
            float r = std::cos(phi) * radius;

            for (int j = 0; j <= segments; ++j)
            {
                float u = static_cast<float>(j) / segments;
                float theta = u * glm::two_pi<float>();

                float x = std::cos(theta) * r;
                float z = std::sin(theta) * r;

                glm::vec3 position(x, y + yOffset, z);

                glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
                
                m_verts.push_back({ position, normal, glm::vec2(u, 1.0f - v) });
            }
        }

        int ringsTotal = rings * 2 + 1;
        for (int i = 0; i < ringsTotal; ++i)
        {
            for (int j = 0; j < segments; ++j)
            {
                int first = (i * (segments + 1)) + j;
                int second = first + segments + 1;

                m_indices.push_back(first);
                m_indices.push_back(second);
                m_indices.push_back(first + 1);

                m_indices.push_back(second);
                m_indices.push_back(second + 1);
                m_indices.push_back(first + 1);
            }
        }
    }
}