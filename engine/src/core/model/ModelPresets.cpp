#include "core/model/ModelPresets.h"

namespace CoreEngine
{
    SimpleSphere_radius1::SimpleSphere_radius1(const float radius, const glm::vec3& color, const glm::vec3& position, const glm::quat& m_rotation, const int subdivisions)
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

    const SimpleSphere_radius1& SimpleSphere_radius1::GetInstance()
    {
        static const SimpleSphere_radius1 sphere_instance(1.0f, glm::vec3(1.0f), glm::vec3(0.0f), glm::identity<glm::quat>(), 4);
        return sphere_instance;
    }
}