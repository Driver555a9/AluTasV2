#pragma once

#include "core/model/Model.h"

namespace CoreEngine
{
    struct SimpleCube_1x1x1
    {
        static inline const std::vector<Vertex> CUBE_VERTICES = {
            // COORDINATES              NORMALS             TEX UV
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
            // Back face
            {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
            // Top face
            {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},
            // Bottom face
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
            // Right face
            {{ 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}}
        };

        static inline const std::vector<GLuint> CUBE_INDICES = {
            // Front face (+Z) - looking at it from outside
            0, 1, 2,
            0, 2, 3,
            
            // Back face (-Z) - looking at it from outside
            5, 4, 7,
            5, 7, 6,
            
            // Top face (+Y) - looking down at it from above
            8, 9, 10,
            8, 10, 11,
            
            // Bottom face (-Y) - looking up at it from below
            13, 12, 15,
            13, 15, 14,
            
            // Left face (-X) - looking at it from outside (left side)
            16, 17, 18,
            16, 18, 19,
            
            // Right face (+X) - looking at it from outside (right side)
            21, 20, 23,
            21, 23, 22
        };
    };

    struct SimpleSphere_radius1
    {
    public:
        std::vector<Vertex> m_sphere_verts;
        std::vector<GLuint> m_sphere_indices;

        static const SimpleSphere_radius1& GetInstance()
        {
            static const SimpleSphere_radius1 sphere_instance(1.0f, 4);
            return sphere_instance;
        }

    private:
        SimpleSphere_radius1(float radius, int subdivisions);
    };

    struct SimpleCylinder_r1_h2
    {
    public:
        std::vector<Vertex> m_verts;
        std::vector<GLuint> m_indices;

        static const SimpleCylinder_r1_h2& GetInstance()
        {
            static const SimpleCylinder_r1_h2 instance(32);
            return instance;

        }
    private:
        SimpleCylinder_r1_h2(int segments) noexcept;
    };

    struct SimpleCapsule_r1_h2
    {
    public:
        std::vector<Vertex> m_verts;
        std::vector<GLuint> m_indices;

        static const SimpleCapsule_r1_h2& GetInstance()
        {
            static const SimpleCapsule_r1_h2 instance(32, 16);
            return instance;
        }
    private:
        SimpleCapsule_r1_h2(int segments, int rings) noexcept;
    };
}
