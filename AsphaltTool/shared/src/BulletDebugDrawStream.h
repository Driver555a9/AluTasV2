#pragma once

#include <cstdint>
#include <type_traits>
#define NOMINMAX
#include "BulletTypes.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <iostream>

namespace BulletTypes
{
    struct DebugDrawStream : IDebugDraw
    {
        struct DrawLineCmd
        {
            Vector3 m_from;
            Vector3 m_to;
            Vector3 m_from_color;
            Vector3 m_to_color;
        };

        struct DrawSphereCmd
        {
            Vector3 m_center;
            float m_radius;
            Transform m_transform;
            Vector3 m_color;
            bool m_has_transform;
        };

        struct DrawTriangleCmd
        {
            Vector3 m_v0, m_v1, m_v2;
            Vector3 m_n0, m_n1, m_n2;
            Vector3 m_color;
            float m_alpha;
            bool m_has_normals;
        };

        struct DrawContactPointCmd
        {
            Vector3 m_point_on_B;
            Vector3 m_normal_on_B;
            float m_distance;
            int m_life_time;
            Vector3 m_color;
        };

        struct DrawAabbCmd
        {
            Vector3 m_from;
            Vector3 m_to;
            Vector3 m_color;
        };

        struct DrawTransformCmd
        {
            Transform m_transform;
            float m_ortho_len;
        };

        struct DrawBoxCmd
        {
            Vector3 m_bb_min;
            Vector3 m_bb_max;
            Transform m_transform;
            Vector3 m_color;
            bool m_has_transform;
        };

        struct DrawCapsuleCmd
        {
            float m_radius;
            float m_half_height;
            int m_up_axis;
            Transform m_transform;
            Vector3 m_color;
        };

        struct DrawCylinderCmd
        {
            float m_radius;
            float m_half_height;
            int m_up_axis;
            Transform m_transform;
            Vector3 m_color;
        };

        struct DrawConeCmd
        {
            float m_radius;
            float m_height;
            int m_up_axis;
            Transform m_transform;
            Vector3 m_color;
        };

        struct DrawPlaneCmd
        {
            Vector3 m_plane_normal;
            float m_plane_const;
            Transform m_transform;
            Vector3 m_color;
        };

        struct DrawCachedMeshInstanceCmd
        {
            uint64_t  m_mesh_id;
            Transform m_transform;
            Vector3   m_scale;
        };

        struct ResetAllDataPseudoCmd
        {

        };

        enum class CaptureMode : uint8_t { Live, CachedMeshDefine };

        struct CachedMeshVertex
        {
            constexpr static uint8_t INVALID_MATERIAL_INDEX = 255;
            Vector3 m_position;
            Vector3 m_normal;
            uint8_t m_material_index = INVALID_MATERIAL_INDEX;
            uint8_t _padd[3];
        };

        constexpr static size_t MAX_VERTS_PER_MESH_CHUNK = 4096;

        struct alignas(64) CachedMeshDefinitionChunk
        {
            uint64_t m_mesh_id {0};
            uint32_t m_chunk_index {0};
            uint32_t m_is_last_chunk {0};
            uint32_t m_vertex_count {0};
            Vector3 m_color {};
            CachedMeshVertex m_vertices[MAX_VERTS_PER_MESH_CHUNK];

            constexpr size_t GetActiveByteSize() const noexcept { return sizeof(CachedMeshDefinitionChunk); }
        };
        static_assert(std::is_trivially_copyable_v<CachedMeshDefinitionChunk>);
        static_assert(std::is_trivially_destructible_v<CachedMeshDefinitionChunk>);

        enum class DrawCmdType : uint8_t
        {
            None = 0,
            Line,
            Sphere,
            Triangle,
            ContactPoint,
            AABB,
            Transform,
            Box,
            Capsule,
            Cylinder,
            Cone,
            Plane,
            CachedMeshInstance,
            ResetAllDataPseudoCmd
        };

        struct alignas(16) DrawCommand
        {
            DrawCmdType m_type { DrawCmdType::None };
            uint8_t _pad[15] {};

            union Data
            {
                DrawLineCmd m_line;
                DrawSphereCmd m_sphere;
                DrawTriangleCmd m_triangle;
                DrawContactPointCmd m_contact_point;
                DrawAabbCmd m_aabb;
                DrawTransformCmd m_transform;
                DrawBoxCmd m_box;
                DrawCapsuleCmd m_capsule;
                DrawCylinderCmd m_cylinder;
                DrawConeCmd m_cone;
                DrawPlaneCmd m_plane;
                DrawCachedMeshInstanceCmd m_cached_instance;
                ResetAllDataPseudoCmd m_reset_pseudo_cmd;

                Data() {}
            } m_data;
        };

        static_assert(std::is_trivially_copyable_v<DrawCommand>);
        static_assert(std::is_trivially_destructible_v<DrawCommand>);

        constexpr static size_t MAX_DRAWS_PER_FRAME = 100'000;

        struct alignas(64) DebugFrameData
        {
            uint32_t m_frame_number {0};
            uint32_t m_command_count {0};
            int m_debug_mode {0};
            uint8_t _pad[52] {};
            DrawCommand m_commands[MAX_DRAWS_PER_FRAME];
            
            [[nodiscard]] constexpr size_t GetActiveByteSize() const noexcept
            {
                const size_t count = (m_command_count > MAX_DRAWS_PER_FRAME) ? MAX_DRAWS_PER_FRAME : m_command_count;
                return offsetof(DebugFrameData, m_commands) + (count * sizeof(DrawCommand));
            }
        };
        static_assert(std::is_trivially_copyable_v<DebugFrameData>);
        static_assert(std::is_trivially_destructible_v<DebugFrameData>);

    protected:
        DefaultColors m_default_colors; 
        std::unique_ptr<DebugFrameData> m_staging_frame = nullptr;
        int m_debug_mode = IDebugDraw::DebugDrawModes::DBG_MAX_DEBUG_DRAW_MODE;
        CaptureMode m_capture_mode = CaptureMode::Live;
        uint64_t m_capturing_mesh_id = 0;
        Vector3 m_capturing_mesh_color {};
        std::vector<CachedMeshVertex> m_capture_buffer;
        std::vector<CachedMeshDefinitionChunk> m_pending_chunks;

        class TriangleExtractorCallback : public BulletTypes::TriangleCallback
        {
        private:
            struct SubpartMaterialCache
            {
                const unsigned char* material_base = nullptr;
                int num_materials = 0;
                PHY_ScalarType material_type;
                int material_stride = 0;
                const unsigned char* triangle_material_base = nullptr;
                int num_triangles = 0;
                int triangle_material_stride = 0;
                PHY_ScalarType triangle_type;
            };

            std::unordered_map<uint8_t, std::vector<CachedMeshVertex>> m_vertices_by_material;
            std::vector<SubpartMaterialCache> m_subpart_caches;
            const BulletTypes::TriangleIndexVertexMaterialArray* m_material_array = nullptr;


        public:
            TriangleExtractorCallback(const BulletTypes::TriangleIndexVertexMaterialArray* material_array = nullptr) noexcept 
            : m_material_array(material_array)
            {
                if (m_material_array)
                {
                    int num_subparts = m_material_array->m_indexed_meshes.m_size;
                    m_subpart_caches.resize(num_subparts);

                    for (int i = 0; i < num_subparts; ++i)
                    {
                        m_material_array->GetLockedReadOnlyMaterialBase(
                            &m_subpart_caches[i].material_base,
                            m_subpart_caches[i].num_materials,
                            m_subpart_caches[i].material_type,
                            m_subpart_caches[i].material_stride,
                            &m_subpart_caches[i].triangle_material_base,
                            m_subpart_caches[i].num_triangles,
                            m_subpart_caches[i].triangle_material_stride,
                            m_subpart_caches[i].triangle_type,
                            i
                        );
                    }
                }
            }

            virtual void ProcessTriangle(const Vector3* triangle_vertices, int subpart_index, int triangle_index) noexcept override
            {
                decltype(CachedMeshVertex::m_material_index) material_index = CachedMeshVertex::INVALID_MATERIAL_INDEX;

                if (m_material_array && subpart_index >= 0 && subpart_index < static_cast<int>(m_subpart_caches.size()))
                {
                    const auto& cache = m_subpart_caches[subpart_index];
                    if (cache.triangle_material_base && triangle_index >= 0 && triangle_index < cache.num_triangles)
                    {
                        const unsigned char* tri_mat_ptr = cache.triangle_material_base + (triangle_index * cache.triangle_material_stride);
                        material_index = static_cast<decltype(material_index)>(*tri_mat_ptr);
                    }
                }

                const Vector3 p0 =  triangle_vertices[0];
                const Vector3 p1 =  triangle_vertices[1];
                const Vector3 p2 =  triangle_vertices[2];

                Vector3 edge1 = p0 - p1;
                Vector3 edge2 = p2 - p1;
                Vector3 normal = edge1.Cross(edge2);
                normal.Normalize();

                auto& bucket = m_vertices_by_material[material_index];
                bucket.emplace_back(p0, normal, material_index);
                bucket.emplace_back(p1, normal, material_index);
                bucket.emplace_back(p2, normal, material_index);
            }

            void ReserveHint(size_t total_estimate) noexcept { m_vertices_by_material.reserve(total_estimate); }
            const std::unordered_map<uint8_t, std::vector<CachedMeshVertex>>& GetVerticesByMaterial() const noexcept { return m_vertices_by_material; }
        };

    public:
        explicit DebugDrawStream()
        {
            m_staging_frame = std::make_unique<DebugFrameData>();
            m_staging_frame->m_command_count = 0;
            m_staging_frame->m_frame_number = 0;
        }

        const DebugFrameData& GetStagingFrameData() const noexcept { return *m_staging_frame; }

        void SetCaptureMode(CaptureMode mode) noexcept { m_capture_mode = mode; }
        [[nodiscard]] CaptureMode GetCaptureMode() const noexcept { return m_capture_mode; }

        void BeginCachedMeshCapture(uint64_t mesh_id, const Vector3& color) noexcept
        {
            m_capturing_mesh_id = mesh_id;
            m_capturing_mesh_color = color;
            m_capture_buffer.clear();
        }

        void EndCachedMeshCapture() noexcept
        {
            if (m_capture_buffer.empty()) return;

            const size_t total = m_capture_buffer.size();
            const uint32_t chunk_count = static_cast<uint32_t>((total + MAX_VERTS_PER_MESH_CHUNK - 1) / MAX_VERTS_PER_MESH_CHUNK);

            for (uint32_t c = 0; c < chunk_count; ++c)
            {
                CachedMeshDefinitionChunk chunk{};
                chunk.m_mesh_id     = m_capturing_mesh_id;
                chunk.m_chunk_index = c;
                chunk.m_color       = m_capturing_mesh_color;

                const size_t begin    = c * MAX_VERTS_PER_MESH_CHUNK;
                const size_t end      = begin + MAX_VERTS_PER_MESH_CHUNK < total ? begin + MAX_VERTS_PER_MESH_CHUNK : total;
                chunk.m_vertex_count  = static_cast<uint32_t>(end - begin);
                chunk.m_is_last_chunk = (c == chunk_count - 1) ? 1u : 0u;

                std::copy(m_capture_buffer.begin() + begin, m_capture_buffer.begin() + end, chunk.m_vertices);
                m_pending_chunks.push_back(chunk);
            }

            m_capture_buffer.clear();
        }

        std::vector<uint64_t> CustomDrawStaticMultiMaterialTriangleMesh(const MultimaterialTriangleMeshShape* multimat_tri, const BulletTypes::TriangleIndexVertexMaterialArray* material_array, 
                                                                        uint64_t base_mesh_id) noexcept
        {
            std::vector<uint64_t> created_ids;
            if (! multimat_tri || ! material_array) return created_ids;

            TriangleExtractorCallback extractor(material_array);
            multimat_tri->ProcessAllTriangles(&extractor, Vector3(-10e9f, -10e9f, -10e9f), Vector3(10e9f, 10e9f, 10e9f));

            const auto HSVtoRGB = [](float h, float s, float v) -> BulletTypes::Vector3
            {
                h = fmod(h, 360.0f);
                float c = v * s;
                float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
                float m = v - c;
                BulletTypes::Vector3 rgb;
                if (h < 60)       rgb = {c, x, 0};
                else if (h < 120) rgb = {x, c, 0};
                else if (h < 180) rgb = {0, c, x};
                else if (h < 240) rgb = {0, x, c};
                else if (h < 300) rgb = {x, 0, c};
                else              rgb = {c, 0, x};
                return rgb + BulletTypes::Vector3(m, m, m);
            }; 

            const auto MaterialIndexToColor = [&HSVtoRGB](uint8_t material_index)
            {
                const float hue = std::fmod(material_index * 137.508f, 360.0f);
                return HSVtoRGB(hue, 0.65f, 0.95f);
            };

            for (const auto& [material_index, verts] : extractor.GetVerticesByMaterial())
            {
                if (verts.empty()) continue;

                const uint64_t sub_mesh_id = base_mesh_id ^ (static_cast<uint64_t>(material_index) << 56);

                BeginCachedMeshCapture(sub_mesh_id, MaterialIndexToColor(material_index));
                m_capture_buffer = verts;
                EndCachedMeshCapture();

                created_ids.push_back(sub_mesh_id);
            }

            return created_ids;
        }

        [[nodiscard]] std::vector<CachedMeshDefinitionChunk> DrainPendingMeshChunks() noexcept
        {
            return std::exchange(m_pending_chunks, {});
        }

        template<typename T>
        inline void PushCommand(DrawCmdType type, const T& cmd_data) noexcept
        {
            if (m_staging_frame->m_command_count >= MAX_DRAWS_PER_FRAME) return;

            DrawCommand& cmd = m_staging_frame->m_commands[m_staging_frame->m_command_count++];
            cmd.m_type = type;
            m_staging_frame->m_debug_mode = m_debug_mode;

            if constexpr (std::is_same_v<T, DrawLineCmd>) cmd.m_data.m_line = cmd_data;
            else if constexpr (std::is_same_v<T, DrawSphereCmd>) cmd.m_data.m_sphere = cmd_data;
            else if constexpr (std::is_same_v<T, DrawTriangleCmd>) cmd.m_data.m_triangle = cmd_data;
            else if constexpr (std::is_same_v<T, DrawContactPointCmd>) cmd.m_data.m_contact_point = cmd_data;
            else if constexpr (std::is_same_v<T, DrawAabbCmd>) cmd.m_data.m_aabb = cmd_data;
            else if constexpr (std::is_same_v<T, DrawTransformCmd>) cmd.m_data.m_transform = cmd_data;
            else if constexpr (std::is_same_v<T, DrawBoxCmd>) cmd.m_data.m_box = cmd_data;
            else if constexpr (std::is_same_v<T, DrawCapsuleCmd>) cmd.m_data.m_capsule = cmd_data;
            else if constexpr (std::is_same_v<T, DrawCylinderCmd>) cmd.m_data.m_cylinder = cmd_data;
            else if constexpr (std::is_same_v<T, DrawConeCmd>) cmd.m_data.m_cone = cmd_data;
            else if constexpr (std::is_same_v<T, DrawPlaneCmd>) cmd.m_data.m_plane = cmd_data;
            else if constexpr (std::is_same_v<T, DrawCachedMeshInstanceCmd>) cmd.m_data.m_cached_instance = cmd_data;
            else if constexpr (std::is_same_v<T, ResetAllDataPseudoCmd>) cmd.m_data.m_reset_pseudo_cmd = cmd_data;
            else { static_assert(sizeof(int) == 0, "Invalid type"); }
        }

        void DrawCachedMeshInstance(uint64_t mesh_id, Transform transform, Vector3 scale)
        {
            PushCommand(DrawCmdType::CachedMeshInstance, DrawCachedMeshInstanceCmd{mesh_id, transform, scale});
        }

        void PushResetAllDataDrawCmd()
        {
            PushCommand(DrawCmdType::ResetAllDataPseudoCmd, ResetAllDataPseudoCmd{});
        }

        virtual ~DebugDrawStream() override = default;

        //virtual DefaultColors GetDefaultColors() const override { return m_default_colors; }
        //virtual void SetDefaultColors(const DefaultColors& colors) override { m_default_colors = colors; }

        virtual void DrawLine(const Vector3& from, const Vector3& to, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Line, DrawLineCmd{ from, to, color, color });
        }

        virtual void DrawLine(const Vector3& from, const Vector3& to, const Vector3& fromColor, const Vector3& toColor) override
        {
            PushCommand(DrawCmdType::Line, DrawLineCmd{ from, to, fromColor, toColor });
        }

        virtual void DrawSphere(float radius, const Transform& transform, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Sphere, DrawSphereCmd{ Vector3{transform.m_origin.x, transform.m_origin.y, transform.m_origin.z}, radius, transform, color, true });
        }

        virtual void DrawSphere(const Vector3& p, float radius, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Sphere, DrawSphereCmd{ p, radius, Transform{}, color, false });
        }

        virtual void DrawTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& n0, const Vector3& n1, const Vector3& n2, const Vector3& color, float alpha) override
        {
            if (m_capture_mode == CaptureMode::CachedMeshDefine)
            {
                m_capture_buffer.push_back({v0, n0});
                m_capture_buffer.push_back({v1, n1});
                m_capture_buffer.push_back({v2, n2});
                return;
            }
            PushCommand(DrawCmdType::Triangle, DrawTriangleCmd{ v0, v1, v2, n0, n1, n2, color, alpha, true });
        }

        virtual void DrawTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& color, float alpha) override
        {
            if (m_capture_mode == CaptureMode::CachedMeshDefine)
            {
                const Vector3 e1{ v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
                const Vector3 e2{ v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };
                Vector3 n{ e1.y*e2.z - e1.z*e2.y, e1.z*e2.x - e1.x*e2.z, e1.x*e2.y - e1.y*e2.x };
                const float len = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
                if (len > 1e-8f) { n.x /= len; n.y /= len; n.z /= len; }

                m_capture_buffer.push_back({v0, n});
                m_capture_buffer.push_back({v1, n});
                m_capture_buffer.push_back({v2, n});
                return;
            }
            PushCommand(DrawCmdType::Triangle, DrawTriangleCmd{ v0, v1, v2, {}, {}, {}, color, alpha, false });
        }

        virtual void DrawContactPoint(const Vector3& PointOnB, const Vector3& normalOnB, float distance, int lifeTime, const Vector3& color) override
        {
            PushCommand(DrawCmdType::ContactPoint, DrawContactPointCmd{ PointOnB, normalOnB, distance, lifeTime, color });
        }

        virtual void ReportErrorWarning(const char* warningString) override {}

        virtual void Draw3dText(const Vector3& location, const char* textString) override {}

        virtual void SetDebugMode(int debug_mode) override { m_debug_mode = debug_mode; }

        virtual int GetDebugMode() const override { return m_debug_mode; }

        virtual void DrawAabb(const Vector3& from, const Vector3& to, const Vector3& color) override
        {
            PushCommand(DrawCmdType::AABB, DrawAabbCmd{ from, to, color });
        }

        virtual void DrawTransform(const Transform& transform, float orthoLen) override
        {
            PushCommand(DrawCmdType::Transform, DrawTransformCmd{ transform, orthoLen });
        }

        virtual void DrawArc(const Vector3&, const Vector3&, const Vector3&, float, float, float, float, const Vector3&, bool, float) override {}

        virtual void DrawSpherePatch(const Vector3&, const Vector3&, const Vector3&, float, float, float, float, float, const Vector3&, float, bool) override {}

        virtual void DrawBox(const Vector3& bbMin, const Vector3& bbMax, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Box, DrawBoxCmd{ bbMin, bbMax, Transform{}, color, false });
        }

        virtual void DrawBox(const Vector3& bbMin, const Vector3& bbMax, const Transform& trans, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Box, DrawBoxCmd{ bbMin, bbMax, trans, color, true });
        }

        virtual void DrawCapsule(float radius, float halfHeight, int upAxis, const Transform& transform, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Capsule, DrawCapsuleCmd{ radius, halfHeight, upAxis, transform, color });
        }

        virtual void DrawCylinder(float radius, float halfHeight, int upAxis, const Transform& transform, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Cylinder, DrawCylinderCmd{ radius, halfHeight, upAxis, transform, color });
        }

        virtual void DrawCone(float radius, float height, int upAxis, const Transform& transform, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Cone, DrawConeCmd{ radius, height, upAxis, transform, color });
        }

        virtual void DrawPlane(const Vector3& planeNormal, float planeConst, const Transform& transform, const Vector3& color) override
        {
            PushCommand(DrawCmdType::Plane, DrawPlaneCmd{ planeNormal, planeConst, transform, color });
        }

        virtual void ClearLines() override
        {
            m_staging_frame->m_command_count = 0;
        }

        virtual void FlushLines() override {}
    };
}