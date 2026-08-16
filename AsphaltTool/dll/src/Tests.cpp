
#include "Communication.h"
#define NOMINMAX
#include "BulletTypes.h"
#include "AsphaltDLLUtility.h"
#include "DetourFunctions.h"
#include "AsphaltDLL.h"
#include "Tests.h"
#include "BulletSerializer.h"

#include <unordered_map>
#include <fstream>
#include <ranges>

namespace AsphaltDLL
{
    namespace Tests
    {
        void PrintCollisionObjectTest(BulletTypes::CollisionObject* obj) noexcept
        {
            if (!obj)
            {
                std::printf("[CollisionObject] NULL Pointer\n");
                return;
            }

            const auto AsFloat3 = [](const BulletTypes::Vector3& v) -> const float* {
                return reinterpret_cast<const float*>(&v);
            };

            std::printf("================ [ CollisionObject @ %p ] ================\n", static_cast<void*>(obj));
            std::printf("  m_vtable_ptr             : %p\n", obj->m_vtable_ptr);
            std::printf("  m_internal_type          : %d (0x%X)\n", obj->m_internal_type, obj->m_internal_type);
            std::printf("  m_collision_flags        : 0x%X\n", obj->m_collision_flags);
            std::printf("  m_activation_state_1     : %d\n", obj->m_activation_state_1);
            std::printf("  m_island_tag / companion : %d / %d (world_idx: %d)\n", obj->m_island_tag_1, obj->m_companion_id, obj->m_world_array_index);
            std::printf("  m_friction / restitution : %.3f / %.3f\n", obj->m_friction, obj->m_restitution);
            std::printf("  m_contact_proc_threshold : %.3f\n", obj->m_contact_processing_threshold);
            const float* aniFric = AsFloat3(obj->m_anisotropic_friction);
            std::printf("  m_anisotropic_friction   : [%.3f, %.3f, %.3f] (has: %d)\n", aniFric[0], aniFric[1], aniFric[2], obj->m_has_anisotropic_friction);
            std::printf("  m_broadphase_proxy_ptr   : %p\n", static_cast<void*>(obj->m_broadphase_proxy_ptr));
            std::printf("  m_collision_shape_ptr    : %p\n", static_cast<void*>(obj->m_collision_shape_ptr));
            std::printf("  m_root_collision_shape   : %p\n", static_cast<void*>(obj->m_root_collision_shape_ptr));
            std::printf("  m_extension_pointer      : %p\n", obj->m_extension_pointer);
            std::printf("  m_user_object_pointer    : %p\n", obj->m_user_object_pointer);
            std::printf("  m_ccd_swept_sphere_rad   : %.3f (thresh: %.3f)\n", obj->m_ccd_swept_sphere_radius, obj->m_ccd_motion_threshold);
            std::printf("  m_hit_fraction           : %.3f\n", obj->m_hit_fraction);
            std::printf("  m_update_revision        : %d\n", obj->m_update_revision);

            if (obj->IsRigidBody())
            {
                auto* rb = static_cast<BulletTypes::RigidBody*>(obj);
                
                std::printf("------------------- [ RigidBody Details ] -------------------\n");
                std::printf("  m_inverse_mass           : %.4f (Mass: %.3f)\n", 
                            rb->m_inverse_mass, (rb->m_inverse_mass > 0.0f ? 1.0f / rb->m_inverse_mass : 0.0f));

                const float* linVel = AsFloat3(rb->m_linear_velocity);
                const float* angVel = AsFloat3(rb->m_angular_velocity);
                std::printf("  m_linear_velocity        : [%.3f, %.3f, %.3f]\n", linVel[0], linVel[1], linVel[2]);
                std::printf("  m_angular_velocity       : [%.3f, %.3f, %.3f]\n", angVel[0], angVel[1], angVel[2]);
                
                const float* grav_acc = AsFloat3(rb->m_gravity_acceleration);
                const float* grav = AsFloat3(rb->m_gravity);
                const float* linFac = AsFloat3(rb->m_linear_factor);
                const float* angFac = AsFloat3(rb->m_angular_factor);
                std::printf("  m_gravity_acceleration   : [%.3f, %.3f, %.3f]\n", grav_acc[0], grav_acc[1], grav_acc[2]);
                std::printf("  m_gravity                : [%.3f, %.3f, %.3f]\n", grav[0], grav[1], grav[2]);
                std::printf("  m_linear_factor          : [%.3f, %.3f, %.3f]\n", linFac[0], linFac[1], linFac[2]);
                std::printf("  m_angular_factor         : [%.3f, %.3f, %.3f]\n", angFac[0], angFac[1], angFac[2]);

                const float* force = AsFloat3(rb->m_total_force);
                const float* torque = AsFloat3(rb->m_total_torque);
                std::printf("  m_total_force            : [%.3f, %.3f, %.3f]\n", force[0], force[1], force[2]);
                std::printf("  m_total_torque           : [%.3f, %.3f, %.3f]\n", torque[0], torque[1], torque[2]);

                std::printf("  m_linear / angular_damp  : %.3f / %.3f\n", rb->m_linear_damping, rb->m_angular_damping);
                std::printf("  m_sleep_thresh (lin/ang) : %.3f / %.3f\n", 
                            rb->m_linear_sleeping_threshold, rb->m_angular_sleeping_threshold);

                const float* pushVel = AsFloat3(rb->m_push_velocity);
                const float* turnVel = AsFloat3(rb->m_turn_velocity);
                std::printf("  m_push_velocity          : [%.3f, %.3f, %.3f]\n", pushVel[0], pushVel[1], pushVel[2]);
                std::printf("  m_turn_velocity          : [%.3f, %.3f, %.3f]\n", turnVel[0], turnVel[1], turnVel[2]);

                std::printf("  m_optional_motion_state  : %p\n", static_cast<void*>(rb->m_optional_motion_state));
                std::printf("  m_rigid_body_flags       : 0x%X\n", rb->m_rigid_body_flags);
                std::printf("  m_debug_body_id          : %d\n", rb->m_debug_body_id);
            }

            std::printf("==========================================================\n\n");
        }


        void ChangeMaterialsTest() noexcept
        {
            auto leaves = DetourFunctions::BVHBroadphaseTraversal::DumpAllLeaves();
            for (auto& leaf : leaves)
            {
                if (! leaf.m_broadphase_proxy || !leaf.m_broadphase_proxy->m_client_body || !leaf.m_broadphase_proxy->m_client_body->m_collision_shape_ptr) continue;
                auto* shape = leaf.m_broadphase_proxy->m_client_body->m_collision_shape_ptr;

                BulletTypes::MultimaterialTriangleMeshShape* multimat = nullptr;
                if (shape->m_shape_type == BulletTypes::BroadphaseNativeTypes::MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE)
                {
                    multimat = reinterpret_cast<BulletTypes::MultimaterialTriangleMeshShape*>(shape);
                }
                else if (shape->m_shape_type == BulletTypes::BroadphaseNativeTypes::SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
                {
                    multimat = reinterpret_cast<BulletTypes::MultimaterialTriangleMeshShape*>(reinterpret_cast<BulletTypes::ScaledBvhTriangleMeshShape*>(shape)->m_bvh_tri_mesh_shape);
                }
                else { continue; }

                if (! multimat->m_mesh_interface->IsInternalTriangleVertexMaterialArray()) continue;
                BulletTypes::TriangleIndexVertexMaterialArray* arr = reinterpret_cast<BulletTypes::TriangleIndexVertexMaterialArray*>(multimat->m_mesh_interface);

                // Pudong
                // 0 = GROUND ROAD
                // 1 = rough terrain
                // 2 = respawn
                // 3 = wall surface
                // 4 = Rough terrain - Walls turn to noclip
                // 5 = Road
                // 6 = Road
                // 7 = Wall surface
                // 8 = Wall surface
                // 9 = Wall surface
                // 10 = Wall surface
                // 11 = Wall surface
                // 12 = Wall surface
                // 13 = Rough terrain - Walls turn to noclip
                // 14 = Very strong rough / Mud - Walls turn to noclip  | In carribean: Wreck
                // 15 = Respawn
                // 16 = Phase through / no contact
                // 17 = Phase through / no contact

                // Carribean extra materials:
                //18 wall
                //19 wall
                //20 wall
                //21 wall
                //22 Mud / Very strong rough
                //23 Flying tree leaves, rough
                //24 mud rough / noclip walls
                //25 tree branches

                constexpr uint8_t FORCED_MATERIAL = 1;

                unsigned char* material_base = nullptr;
                unsigned char* triangle_material_base = nullptr;

                int num_materials = 0;
                int material_stride = 0;
                int num_triangles = 0;
                int triangle_material_stride = 0;

                BulletTypes::PHY_ScalarType material_type;
                BulletTypes::PHY_ScalarType triangle_type;

                for (int subpart = 0; subpart < arr->m_indexed_meshes.m_size; ++subpart)
                {
                    arr->GetLockedMaterialBase(&material_base, num_materials, material_type, material_stride, &triangle_material_base, num_triangles, 
                                                triangle_material_stride, triangle_type, subpart);

                    for (int tri = 0; tri < num_triangles; ++tri)
                    {
                        auto* p = triangle_material_base + tri * triangle_material_stride;
                        *p = FORCED_MATERIAL;
                        /*std::array<unsigned char, 7> roads = {0, 5, 6, 1, 4, 13, 14};
                        std::array<unsigned char, 7> walls = {3, 7, 8, 9, 10, 11, 12};
                        bool changed = false;
                        for(int i{}; i < roads.size(); i++)
                        {
                            if (*p == roads[i])
                            {
                                *p = 8;
                                changed = true;
                                break;
                            }
                        }
                        if (!changed)
                        {
                            for (int i{}; i < walls.size(); i++)
                            {
                                if (*p == walls[i])
                                {
                                    *p = 0;
                                    break;
                                }
                            }
                        }*/
                    }
                }

            }

            const uintptr_t racer_address = GameDLLState::g_current_state.m_resolved_addresses.m_local_racer_base_address;
            if (racer_address != NO_VALID_RESOLVED_ADDRESS)
            {
                BulletTypes::Transform trans {};
                std::memcpy(trans.Data(), reinterpret_cast<void*>(racer_address + ComDllIn::WriteRacerState::OFFSET_TRANSFORM), sizeof(BulletTypes::Transform));

                trans.m_origin = {1947.0f, 362, 40.9f};

                std::memcpy(reinterpret_cast<void*>(racer_address + ComDllIn::WriteRacerState::OFFSET_TRANSFORM), trans.Data(), sizeof(trans));

                BulletTypes::Vector3 velo;
                std::memcpy(velo.Data(), reinterpret_cast<void*>(racer_address + ComDllIn::WriteRacerState::OFFSET_VELOCITY), sizeof(BulletTypes::Vector3));
                velo *= 20;
                std::memcpy(reinterpret_cast<void*>(racer_address + ComDllIn::WriteRacerState::OFFSET_VELOCITY), velo.Data(), sizeof(velo));
            }
        }

    namespace  
    {
        bool PrintProxyInfo(std::ofstream& log, BulletTypes::BroadphaseProxy* proxy) noexcept
        {
            if (!proxy)
            {
                log << "NULL PROXY\n\n";
                return false;
            }

            log << "////////////// BroadphaseProxy //////////////\n";
            log << "client_body: "  << proxy->m_client_body << "\n";
            log << "filter_group: " << proxy->m_collision_filter_group << "\n";
            log << "filter_mask: "  << proxy->m_collision_filter_mask << "\n";
            log << "unique_id: " << proxy->m_unique_id << "\n";
            log << "AABB Min: " << proxy->m_aabb_min.ToString() << "\n";
            log << "AABB Max: " << proxy->m_aabb_max.ToString() << "\n\n";

            return true;
        }

        bool PrintCollisionObjectInfo(std::ofstream& log, BulletTypes::CollisionObject* object) noexcept
        {
            if (!object)
            {
                log << "NULL CollisionObject\n\n";
                return false;
            }

            log << "////////////// CollisionObject //////////////\n";
            log << std::hex << std::showbase;
            log << "vtable: " << object->m_vtable_ptr << "\n";
            log << "broadphase_proxy_ptr: " << object->m_broadphase_proxy_ptr << "\n";
            log << "collision_shape_ptr: " << object->m_collision_shape_ptr << "\n";
            log << "root_collision_shape_ptr: "<< object->m_root_collision_shape_ptr << "\n";
            log << "extensionPointer: " << object->m_extension_pointer << "\n";
            log << std::dec << std::noshowbase;
            log << "\nTransforms:\n";
            log << "World:\n" << object->m_transform_matrix.ToString()<< "\n";
            log << "Interpolation World:\n" << object->m_interpolation_world_transform.ToString() << "\n";
            log << "\nVectors:\n";
            log << "Linear Velocity: " << object->m_interpolation_linear_velocity.ToString() << "\n";
            log << "Angular Velocity: "<< object->m_interpolation_angular_velocity.ToString() << "\n";
            log << "Anisotropic Friction: " << object->m_anisotropic_friction.ToString()<< "\n";
            log << "\nFlags:\n";
            log << "has_anisotropic_friction: " << object->m_has_anisotropic_friction << "\n";
            log << "collision_flags: " << object->m_collision_flags << "\n";
            log << "island_tag_1: " << object->m_island_tag_1 << "\n";
            log << "activation_state: " << object->m_activation_state_1 << "\n";
            log << "internal_type: " << object->m_internal_type << "\n";
            log << "\nMaterial:\n";
            log << "friction: " << object->m_friction << "\n";
            log << "restitution: " << object->m_restitution << "\n";
            log << "\nUser:\n";
            log << "user_object_pointer: "<< object->m_user_object_pointer << "\n";
            log << "\nCCD:\n";
            log << "hit_fraction: " << object->m_hit_fraction << "\n";
            log << "ccd_swept_sphere_radius: " << object->m_ccd_swept_sphere_radius << "\n";
            log << "ccd_motion_threshold: " << object->m_ccd_motion_threshold << "\n";
            log << "\nCollision Filtering:\n";
            log << "check_collide_with: " << object->m_check_collide_with << "\n";
            log << "objects_without_collision_check size: " << object->m_objects_without_collision_check.m_size << "\n";
            log << "objects_without_collision_check capacity: "<< object->m_objects_without_collision_check.m_capacity << "\n";

            if (object->m_collision_shape_ptr && object->m_collision_shape_ptr->m_shape_type == BulletTypes::BroadphaseNativeTypes::MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE)
            {
                auto* multi = reinterpret_cast<BulletTypes::MultimaterialTriangleMeshShape*>(object->m_collision_shape_ptr);
                log << "Amount Triangles: " << multi->GetAmountTriangles() << "\n";
            }
            log << "Type: " << object->m_collision_shape_ptr->GetName();

            return true;
        }

        bool PrintCollisionShapeInfo(std::ofstream& log, BulletTypes::CollisionShape* shape) noexcept
        {
            if (!shape)
            {
                log << "NULL CollisionShape\n\n";
                return false;
            }

            log << std::hex
                << "\n////////////// Collision Shape //////////////"
                << "\nAddress: " << shape
                << "\nVtable:  " << shape->m_vtable_ptr
                << std::dec
                << "\nType:    " << shape->m_shape_type
                << "\n";

            if (const auto* box = BulletTypes::SafeShapeCast<const BulletTypes::BoxShape>(shape))
            {
                log << "\n////////////// BOX SHAPE //////////////"
                    << "\nSize: " << sizeof(BulletTypes::BoxShape)
                    << "\nImplicit dimensions: " << box->m_implicit_shape_dimensions.ToString()
                    << "\nLocal scaling: "  << box->m_local_scaling.ToString()
                    << "\nMargin: " << box->m_collision_margin
                    << "\n";
            }
            else if (const auto* scaled = BulletTypes::SafeShapeCast<const BulletTypes::ScaledBvhTriangleMeshShape>(shape))
            {
                log << "\n////////////// SCALED BVH TRIANGLE MESH //////////////"
                    << "\nSize: " << sizeof(BulletTypes::ScaledBvhTriangleMeshShape)
                    << "\nLocal scaling: " << scaled->m_local_scaling.ToString()
                    << "\nBvh mesh ptr: " << std::hex << scaled->m_bvh_tri_mesh_shape << std::dec
                    << "\nMargin: " << scaled->m_collision_margin
                    << "\n";

                if (scaled->m_bvh_tri_mesh_shape)
                {
                    auto* mesh = scaled->m_bvh_tri_mesh_shape;

                    log << "\n  ////////////// Underlying BVH Mesh //////////////"
                        << "\n  Vtable: " << std::hex << mesh->m_vtable_ptr << "\n  Bvh ptr: " << mesh->m_bvh
                        << "\n  Mesh interface: " << mesh->m_mesh_interface << std::dec
                        << "\n  Local AABB Min: " << mesh->m_local_aabb_min.ToString()
                        << "\n  Local AABB Max: " << mesh->m_local_aabb_max.ToString()
                        << "\n  Interface Has AABB: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_has_aabb
                        << "\n  Interface AABB Min: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_aabb_min.ToString()
                        << "\n  Interface AABB Max: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_aabb_max.ToString()
                        << "\n  VertexArrayMeshes Data: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_indexed_meshes.m_data
                        << "\n  VertexArrayMeshes Size: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_indexed_meshes.m_size
                        << "\n  VertexArrayMeshes Capa: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->m_indexed_meshes.m_capacity
                        << "\n  Amount Triangles: " << reinterpret_cast<const BulletTypes::TriangleIndexVertexArray*>(mesh->m_mesh_interface)->GetAmountTriangles();

                    if (mesh->m_mesh_interface)
                    {
                         log << "\n    /// Striding Mesh Interface ///"
                            << "\n   Vtable: " << std::hex << mesh->m_mesh_interface->m_vtable_ptr
                            << "\n   Scaling: " << mesh->m_mesh_interface->m_scaling.ToString()
                          /*  << "\n   HasAABB: " << mesh->m_mesh_interface->m_has_aabb
                            << "\n   AABB Min: " << mesh->m_mesh_interface->m_aabb_min.ToString()
                            << "\n   AABB Max: " << mesh->m_mesh_interface->m_aabb_max.ToString()
                            << "\n   Array Data: " << mesh->m_mesh_interface->m_indexed_meshes.m_data 
                            << "\n   Array Size: " << mesh->m_mesh_interface->m_indexed_meshes.m_size
                            << "\n   Array Capacity: " << mesh->m_mesh_interface->m_indexed_meshes.m_capacity*/;
                    }
                    
                    if (mesh->m_bvh)
                    {
                        log << "    ///\nOptimized BVH ///\n"
                            << "    AABB Min: " << mesh->m_bvh->m_bvh_aabb_min.ToString() << "\n"
                            << "    AABB Max: " << mesh->m_bvh->m_bvh_aabb_max.ToString() << "\n"
                            << "    Use Quantization: " << mesh->m_bvh->m_use_quantization << "\n"
                            << "    Node Count (Leaf): " << mesh->m_bvh->m_leaf_nodes.m_size << "\n"
                            << "    Quantized Node Count: " << mesh->m_bvh->m_quantized_leaf_nodes.m_size << "\n";
                    }
                }
            }
            else if (const auto* multi = BulletTypes::SafeShapeCast<const BulletTypes::MultimaterialTriangleMeshShape>(shape))
            {
                log << "\n////////////// MULTIMATERIAL TRIANGLE MESH //////////////"
                    << "\nSize: " << sizeof(BulletTypes::MultimaterialTriangleMeshShape)
                    << "\nBvh ptr: " << std::hex << multi->m_bvh
                    << "\nMesh Interface: " << multi->m_mesh_interface 
                    << "\nMaterial array ptr: " << multi->m_material_list.m_data
                    << "\nTriangleInfoMap ptr: " << multi->m_triangle_info_map << std::dec
                    << "\nMaterial count: " << multi->m_material_list.m_size
                    << "\nMaterial capacity: " << multi->m_material_list.m_capacity
                    << "\nUseQuantAabbCompress: " << static_cast<int>(std::bit_cast<uint8_t>(multi->m_use_quantized_aabb_compression))
                    << "\nOwnsBvh: " << multi->m_owns_bvh << "\n";

                /*if (multi->m_bvh)
                {
                     log << "  /// Optimized BVH ///\n"
                         << "  AABB Min: " << multi->m_bvh->m_bvh_aabb_min.ToString() << "\n"
                         << "  AABB Max: " << multi->m_bvh->m_bvh_aabb_max.ToString() << "\n"
                         << "  BvhQuantization: " << multi->m_bvh->m_bvh_quantization.ToString() << "\n"
                         << "  Node Count (Leaf): " << multi->m_bvh->m_leaf_nodes.m_size << " Capacity: " << multi->m_bvh->m_leaf_nodes.m_capacity << "\n" 
                         << "  Node Count (Contiguous): " << multi->m_bvh->m_contiguous_nodes.m_size << " Capacity: " << multi->m_bvh->m_contiguous_nodes.m_capacity << "\n"
                         << "  Node Count (QuantLeafNodes): " << multi->m_bvh->m_quantized_leaf_nodes.m_size << "Capacity: " << multi->m_bvh->m_contiguous_nodes.m_capacity << "\n"
                         << "  Node Count (QuantContiguous): " << multi->m_bvh->m_quantized_contiguous_nodes.m_size << "Capacity: " << multi->m_bvh->m_quantized_contiguous_nodes.m_capacity << "\n"
                         << "  Traversal mode: " << multi->m_bvh->m_traversal_mode
                         << "  Use Quantization: " << static_cast<int>(std::bit_cast<uint8_t>(multi->m_bvh->m_use_quantization));
                }*/

                if (multi->m_mesh_interface)
                {
                    const BulletTypes::TriangleIndexVertexMaterialArray* arr =
                        reinterpret_cast<const BulletTypes::TriangleIndexVertexMaterialArray*>(multi->m_mesh_interface);

                    unsigned char* material_base = nullptr;
                    unsigned char* triangle_material_base = nullptr;

                    int num_materials = 0;
                    int material_stride = 0;
                    int num_triangles = 0;
                    int triangle_material_stride = 0;

                    BulletTypes::PHY_ScalarType material_type;
                    BulletTypes::PHY_ScalarType triangle_type;

                    const_cast<BulletTypes::TriangleIndexVertexMaterialArray*>(arr)->GetLockedMaterialBase(
                        &material_base,
                        num_materials,
                        material_type,
                        material_stride,
                        &triangle_material_base,
                        num_triangles,
                        triangle_material_stride,
                        triangle_type);

                    log << "\n=== Material Info ===\n";
                    log << "material_base            = " << static_cast<const void*>(material_base) << '\n';
                    log << "num_materials            = " << num_materials << '\n';
                    log << "material_stride          = " << material_stride << '\n';
                    log << "material_type            = " << static_cast<int>(material_type) << '\n';
                    log << "triangle_material_base   = " << static_cast<const void*>(triangle_material_base) << '\n';
                    log << "num_triangles            = " << num_triangles << '\n';
                    log << "triangle_material_stride = " << triangle_material_stride << '\n';
                    log << "triangle_type            = " << static_cast<int>(triangle_type) << "\n\n";

                    for (int i = 0; i < num_materials; ++i)
                    {
                        const unsigned char* p = material_base + i * material_stride;

                        log << "Material[" << i << "]: ";

                        for (int j = 0; j < material_stride; ++j)
                        {
                            log     << std::hex
                                    << std::setw(2)
                                    << std::setfill('0')
                                    << static_cast<int>(p[j])
                                    << ' ';
                        }

                        log << std::dec << '\n';
                    }

                    log << '\n';

                    for (int i = 0; i < std::min(num_triangles, 10); ++i)
                    {
                        const unsigned char* p = triangle_material_base + i * triangle_material_stride;

                        log << "TriangleMaterial[" << i << "]: ";

                        for (int j = 0; j < triangle_material_stride; ++j)
                        {
                            log << std::hex
                                    << std::setw(2)
                                    << std::setfill('0')
                                    << static_cast<int>(p[j])
                                    << ' ';
                        }

                        log << std::dec << '\n';
                    }
                }
            }
            else if (const auto* compound = BulletTypes::SafeShapeCast<const BulletTypes::CompoundShape>(shape))
            {
                log << "\n////////////// COMPOUND SHAPE //////////////"
                    << "\nSize: " << sizeof(BulletTypes::CompoundShape)
                    << "\nChildren ptr: " << std::hex << compound->m_children.m_data << std::dec
                    << "\nChild count: " << compound->m_children.m_size
                    << "\nChild capacity: " << compound->m_children.m_capacity
                    << "\nAABB Min: " << compound->m_local_aabb_min.ToString()
                    << "\nAABB Max: " << compound->m_local_aabb_max.ToString()
                    << "\nScaling: "  << compound->m_local_scaling.ToString()
                    << "\nTree ptr: " << std::hex << compound->m_dynamic_aabb_tree << std::dec
                    << "\nDbvt Tree Leaves: " << compound->m_dynamic_aabb_tree->m_leaves
                    << "\nCollisionMargin: " << compound->m_collision_margin
                    << "\n";

                if (compound->m_children.m_data && compound->m_children.m_size > 0 && compound->m_children.m_size < 10000)
                {
                    for (int i = 0; i < std::min(compound->m_children.m_size, 10); i++)
                    {
                        const auto& child = compound->m_children.m_data[i];
                        log << "\n  --- Child " << i << " ---"
                            << "\n  Shape ptr: " << std::hex << child.m_child_shape << std::dec
                            << "\n  Type ID: " << child.m_child_shape_type
                            << "\n  Margin: " << child.m_child_margin
                            << "\n  Node ptr: " << std::hex << child.m_node << std::dec
                            << "\n  Transform Origin: " << child.m_transform.m_origin.ToString()
                            << "\n  Transform: " << child.m_transform.ToString()
                            << "\n  Child Vtable: " << std::hex << child.m_child_shape->m_vtable_ptr << std::dec
                            << "\n";
                    }
                }
                else if (compound->m_children.m_size >= 10000 || compound->m_children.m_size < 0)
                {
                    log << "  [!] Child count looks like garbage data. ABI mismatch likely around m_children offset.\n";
                }
            }
            else if (const auto* sphere = BulletTypes::SafeShapeCast<const BulletTypes::SphereShape>(shape))
            {
                log << "\n////////////// SPHERE SHAPE //////////////"
                    << "\nSize: "
                    << sizeof(BulletTypes::SphereShape)
                    << "\nRadius: "
                    << sphere->m_implicit_shape_dimensions.x
                    << "\nDimensions: "
                    << sphere->m_implicit_shape_dimensions.ToString()
                    << "\nScaling: "
                    << sphere->m_local_scaling.ToString()
                    << "\n";
            }
            else
            {
                log << "\n////////////// UNKNOWN SHAPE //////////////\n";
            }

            return true;
        }
    }
        void DebugDumpPhysicsWorldObjects(const std::string& path) noexcept
        {
            static int counter = 0;
            if (counter++ == 10) // do this once for performance
            {
                const auto leaves = DetourFunctions::BVHBroadphaseTraversal::DumpAllLeaves();
                std::ofstream log(path.c_str(), std::ios::out | std::ios::trunc);

                for (size_t i = 0; i < leaves.size(); ++i)
                {
                    const DetourFunctions::BVHBroadphaseTraversal::DumpedNode& leaf = leaves[i];

                    if (! leaf.m_is_from_static_tree) continue; // Skip dynamics for now
                    
                    log << std::dec << "\n///////////////////////////////////////////////////\n";
                    log << "LEAF " << i << "\n";
                    log << "///////////////////////////////////////////////////\n\n";

                    PrintProxyInfo(log, leaf.m_broadphase_proxy) 
                    && PrintCollisionObjectInfo(log, leaf.m_broadphase_proxy->m_client_body) 
                    && PrintCollisionShapeInfo(log, leaf.m_broadphase_proxy->m_client_body->m_collision_shape_ptr);
                    
                    if ((i % 10) == 0) log.flush();
                }
                log.close();
            }
        }

        void MovePhysicsObjectsTest() noexcept
        {
            static auto leaves = DetourFunctions::BVHBroadphaseTraversal::DumpAllLeaves();

            static float z_offset = 0.000f;
            static std::unordered_map<BulletTypes::CollisionObject*, BulletTypes::Transform> original_trans;

            z_offset += 2.0f / 60.0f;
            z_offset = std::min(50.0f, z_offset);

            for (const auto& leaf : leaves)
            {

                const auto* proxy = leaf.m_broadphase_proxy;
                if (!proxy)
                {
                    DLL_ERROR_PRINT("Err no proxy: ");
                    continue;
                }

                auto* object = proxy->m_client_body;
                if (!object)
                {
                    DLL_ERROR_PRINT("Err no object: Proxy: " << std::hex << proxy << std::dec );
                    continue;
                }

                auto* shape = object->m_collision_shape_ptr;
                if (!shape)
                {
                    DLL_ERROR_PRINT("Err no shape: Proxy: " << std::hex << proxy << std::dec);
                    continue;
                }

                if (shape->m_shape_type != BulletTypes::BroadphaseNativeTypes::MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE) 
                {
                    continue;
                }

                BulletTypes::Vector3 center;
                float radius;
                shape->GetBoundingSphere(center, radius);
                if (radius > 500) continue; // filter out work

                if (!original_trans.contains(object))
                {
                    original_trans.emplace(object, object->m_transform_matrix);
                }
                
                auto& transform = object->m_transform_matrix;
                transform.At(14) = original_trans[object].At(14) + z_offset;
                auto& interp_trans = object->m_interpolation_world_transform;
                interp_trans.At(14) = original_trans[object].At(14) + z_offset;
            }
        }

        void RaycastTest() noexcept
        {
            using namespace DetourFunctions::Experimental::PhysicsWorldRaycast;

            BulletTypes::Vector3 ray_start {};
            BulletTypes::Vector3 ray_end {};

            {
                LOCK_CURRENT_STATE_MUTEX();
                auto trans = std::bit_cast<BulletTypes::Transform>(GameDLLState::g_current_state.m_racer_state.m_racer_transform_mat4x4);
                const BulletTypes::Vector3        racer_pos           = {trans.m_origin.x, trans.m_origin.y, trans.m_origin.z};
                const BulletTypes::Quaternion     racer_rotation      = Utility::RotationFromTransform(trans);
                const BulletTypes::Vector3        world_space_forward = Utility::RotateVectorByQuaternion(racer_rotation, { 0.0f, 1.0f, 0.0f});
                const BulletTypes::Vector3        world_space_up      = Utility::RotateVectorByQuaternion(racer_rotation, { 0.0f, 0.0f, 1.0f}); 
                
                ray_start = racer_pos + world_space_forward * 2.0f  + world_space_up * 1.0f;
                ray_end   = racer_pos + world_space_forward * 10.0f + world_space_up * 1.0f;
            }

            uint16_t layer_mask = 0xFFFF;
            uint16_t flags      = 0xFFFF;

            BulletTypes::RaycastOutput output = SpoofCallToCastRay(ray_start, ray_end, layer_mask, flags);

            DLL_INFO_LOG
            (
                "\nHas Hit : " << output.m_has_hit
                << "\nBody*   : " << std::hex << output.m_hit_body_ptr << std::dec
                << "\nNormal  : " << output.m_hit_normal.x   << ", " << output.m_hit_normal.y << ", " << output.m_hit_normal.z
                << "\nPosition: " << output.m_hit_position.x << ", " << output.m_hit_position.y << ", " << output.m_hit_position.z
                << "\nActi-fil: " << output.m_activation_filter
                << "\nUnknown4: " << output.m_unkown_4
                << "\nUnknown8: " << output.m_unkown_8
            );
        }
    }
}