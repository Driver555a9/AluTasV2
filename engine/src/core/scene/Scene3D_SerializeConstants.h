#pragma once

namespace CoreEngine
{
    namespace _SerializeKey 
    {
        namespace Meta
        {
            inline constexpr char const* Root            = "meta";
            inline constexpr char const* Version         = "version";
            inline constexpr char const* TimestampNow    = "unix_time_now";

            namespace Value
            {
                inline constexpr char const* ManifestVersion = "1.0";
            }
        }

        namespace SceneInfo
        {
            inline constexpr char const* Root            = "scene_info";
            inline constexpr char const* AmountObjects   = "amount_objects";
            inline constexpr char const* AmountLights    = "amount_lights";
        }

        namespace SceneData
        {
            inline constexpr char const* Root            = "scene_data";
            inline constexpr char const* SceneObjects    = "scene_objects";
            inline constexpr char const* LightSources    = "light_sources";
        }

        namespace Camera
        {
            inline constexpr char const* Root      = "camera";
            inline constexpr char const* Position  = "c_pos";
            inline constexpr char const* Rotation  = "c_rot";
            inline constexpr char const* FovDeg    = "c_fov";
            inline constexpr char const* NearPlane = "c_near";
            inline constexpr char const* FarPlane  = "c_far";
        }

        namespace Render 
        {
            inline constexpr char const* Root           = "r_render";            
            inline constexpr char const* Type           = "r_type";           // Type of the model --- relevant for object specific data
            inline constexpr char const* Scale          = "r_render_scale";   // This is executed on GPU when applying model matrix
            inline constexpr char const* Position       = "r_position";
            inline constexpr char const* Rotation       = "r_rotation";

            //Specific attributes for specific model types
            inline constexpr char const* FilePath       = "r_filepath";       // Path of a model loaded from file
            inline constexpr char const* NaturalScale   = "r_natural_scale";  // One time CPU scale of path model when loaded

            inline constexpr char const* PrimitiveColor = "r_color";          //colores for primitives
            inline constexpr char const* HalfExtents    = "r_half_extents";   //extents of box model
            inline constexpr char const* Radius         = "r_radius";         //radius  of sphere model

            inline constexpr char const* Meshes         = "r_meshes";         //Meshes   of point model --- vertices and indices are *per mesh*
            inline constexpr char const* MeshVertices   = "r_vertices";       //Vertices of point model --- Will be packed tightly 
            inline constexpr char const* MeshIndices    = "r_indices";        //indices  of point model
            
            namespace Value 
            {
                inline constexpr char const* IsBoxModel      = "r_is_box";
                inline constexpr char const* IsSphereModel   = "r_is_sphere";
                inline constexpr char const* IsFilePathModel = "r_is_filepath_model";
                inline constexpr char const* IsPointModel    = "r_is_point_model";
            }
        }

        namespace Physics 
        {
            inline constexpr char const* Root            = "p_physics";
            inline constexpr char const* Shape           = "p_shape";
            inline constexpr char const* Scale           = "p_scale";
            inline constexpr char const* Mass            = "p_mass";
            inline constexpr char const* ObjectType      = "p_obj_type";
            inline constexpr char const* Position        = "p_position";
            inline constexpr char const* Rotation        = "p_rotation";

            //Specific attributes for specific shape types
            inline constexpr char const* BoxHalfExtents      = "p_half_extents";  //Box

            inline constexpr char const* SphereRadius        = "p_radius";        //Sphere

            inline constexpr char const* MeshVertexPositions = "p_phys_vertices"; //Vertices of triangle_mesh shape --- 3 triangles consecutively = 1 triangle

            namespace Value 
            {
                inline constexpr char const* Box          = "p_is_box";
                inline constexpr char const* Sphere       = "p_is_sphere";
                inline constexpr char const* TriangleMesh = "p_is_triangle_mesh";

                inline constexpr char const* ObjectTypeOrdinary = "p_is_ordinary";
                inline constexpr char const* ObjectTypeCar      = "p_is_car";
            }
        }

        namespace Light
        {
            inline constexpr char const* Color      = "l_color";
            inline constexpr char const* Intensity  = "l_intensity";
            inline constexpr char const* Mode       = "l_light_mode";
            inline constexpr char const* Position   = "l_position";
        }

        namespace SceneObject
        {
            inline constexpr char const* Name     = "name";
        }
    }
}