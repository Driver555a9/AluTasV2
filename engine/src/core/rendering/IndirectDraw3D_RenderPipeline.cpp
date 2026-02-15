#include "core/rendering/IndirectDraw3D_RenderPipeline.h"

#include "core/rendering/BindingPoints.h"

#include "core/utility/Assert.h"
#include "core/utility/MathUtility.h"
#include "core/utility/Performance.h"

namespace CoreEngine
{
    IndirectDraw3D_RenderPipeline::IndirectDraw3D_RenderPipeline() noexcept
    :   m_shader_program(s_VERTEX_SHADER_CODE, s_FRAGMENT_SHADER_CODE, Shader::ProvidedPointers::ARE_SOURCE_CODE),
        m_ssbo_sizes_ubo(nullptr, sizeof(SSBO_SizesData), UBO_BINDING::IndirectDraw3D_SSBO_SIZES),
        m_camera_ubo(nullptr, sizeof(CameraRenderData), UBO_BINDING::IndirectDraw3D_CAMERA)
    {   
        m_vao.Bind();
        m_ebo.Bind();

        m_vao.LinkAttribute(m_vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
        m_vao.LinkAttribute(m_vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));
        m_vao.LinkAttribute(m_vbo, 2, 2, GL_FLOAT, sizeof(Vertex), (void*)(6 * sizeof(float)));

        m_ebo.Unbind();

        SetSceneData({}, {});
    }

    void IndirectDraw3D_RenderPipeline::Render() noexcept
    {
    //------------------- Depth Testing for 3D
        glDepthMask(GL_TRUE); 
        glEnable(GL_DEPTH_TEST);

        //Reverse Z matrices
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDepthFunc(GL_GREATER);
        glClearDepth(0.0);

        glClear(GL_DEPTH_BUFFER_BIT);
    //------------------- Cull face for performance
        glFrontFace(GL_CCW); 
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);

    //-------------------  Bind buffers
        m_indirect_command_buffer.Bind();
        m_vao.Bind();  //Remember: VAO binds VBO and EBO implicitly already!
        m_camera_ubo.BindBase();

        m_active_draw_indices_ssbo.BindBase();
        m_mesh_transform_ssbo.BindBase();
        m_texture_indices_ssbo.BindBase();
        m_light_ssbo.BindBase();

        m_shader_program.Activate();

        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, m_draw_command_count, 0);

    //------------------- Unbind buffers to avoid othe pipelines having them active
        m_shader_program.Deactivate();
        
        m_indirect_command_buffer.Unbind();
        m_vao.Unbind();
        m_camera_ubo.Unbind();

    }

    void IndirectDraw3D_RenderPipeline::SetSceneData(const std::vector<Basic_Model*>& model_vec, const std::vector<Light>& lights_vec) noexcept
    {
    //------------------ Calcualting these up front to avoid resizing later
        size_t amount_meshes   {};
        size_t amount_vertices {};
        size_t amount_indices  {};
        for (const Basic_Model* model_ptr : model_vec)
        {
            const std::vector<Mesh>& meshes = model_ptr->GetMeshVectorConstReference();
            amount_meshes += meshes.size();
            for (const Mesh& mesh : meshes)
            {
                amount_vertices += mesh.GetVerticesConstReference().size();
                amount_indices  += mesh.GetIndicesConstReference().size();
            }
        } 

    //------------------ Persistent member data
        m_active_draw_indices.clear();
        m_mesh_transforms.clear();
        m_material_ptrs.clear();

        m_active_draw_indices.reserve(amount_meshes);
        m_mesh_transforms.reserve(amount_meshes);
        m_material_ptrs.reserve(amount_meshes);

    //------------------ Temporary data to be passed into GPU buffers
        std::vector<DrawElementsIndirectCommand> temp_draw_commands;
        std::vector<Vertex>   temp_mesh_vertices;
        std::vector<GLuint>   temp_mesh_indices;
        std::vector<MaterialPBR::GPU_std430_Aligned_Data> temp_material;

        temp_draw_commands.reserve(amount_meshes);
        temp_mesh_vertices.reserve(amount_vertices);
        temp_mesh_indices.reserve(amount_indices);
        temp_material.reserve(amount_meshes);

        GLuint offset_indices  = 0;
        GLint  offset_vertices = 0;
        GLuint offset_mesh     = 0;
    //------------------

        for(Basic_Model* model_ptr : model_vec)
        {
            const glm::mat4 model_matrix = model_ptr->GetModelMatrix();

            for (Mesh& mesh : model_ptr->GetMeshVectorReference()) 
            {
                m_active_draw_indices.push_back(offset_mesh);
                m_mesh_transforms.push_back(model_matrix);
                ENGINE_ASSERT(mesh.GetMaterialSharedPtr() && "At IndirectDraw3D::SetSceneData(): Mesh should have a non null material ptr.");
                m_material_ptrs.push_back(mesh.GetMaterialSharedPtr());

                const std::vector<Vertex>& verts   = mesh.GetVerticesConstReference();
                const std::vector<GLuint>& indices = mesh.GetIndicesConstReference();

                //Update vectors to pass to GPU later
                temp_mesh_vertices.insert(temp_mesh_vertices.end(), verts.begin(), verts.end());
                temp_mesh_indices.insert(temp_mesh_indices.end(), indices.begin(), indices.end());

                temp_material.push_back(mesh.GetMaterialConstSharedPtr()->GetGPUAlignedData());

                //Create draw command
                DrawElementsIndirectCommand cmd;
                cmd.count = mesh.GetIndicesConstReference().size();  // Number of indices to draw
                cmd.instanceCount = 1;                               // One instance per model (can be >1 for instancing)
                cmd.firstIndex = offset_indices;                     // Offset in index buffer
                cmd.baseVertex = offset_vertices;                    // Offset in vertex buffer
                cmd.baseInstance = offset_mesh;                      // Model index for SSBO lookup

                temp_draw_commands.push_back(cmd);

                //Update offsets for next cmd
                offset_indices  += indices.size();
                offset_vertices += verts.size();
                offset_mesh     += 1;
            }
        }

        //Set indirect cmd data and save amount of commands
        m_indirect_command_buffer.SetNewData(temp_draw_commands.data(), temp_draw_commands.size() * sizeof(DrawElementsIndirectCommand));
        m_draw_command_count = temp_draw_commands.size();

        //Set SSBO model specific data
        m_texture_indices_ssbo.SetNewData(temp_material.data(), sizeof(MaterialPBR::GPU_std430_Aligned_Data) * temp_material.size(), SSBO_BINDING::IndirectDraw3D_MATERIAL);
        m_mesh_transform_ssbo.SetNewData(m_mesh_transforms.data(), sizeof(glm::mat4) * m_mesh_transforms.size(), SSBO_BINDING::IndirectDraw3D_TRANSFORM);
        m_active_draw_indices_ssbo.SetNewData(m_active_draw_indices.data(), sizeof(GLuint) * m_active_draw_indices.size(), SSBO_BINDING::IndirectDraw3D_DRAW_INDICES);

        //Store Lights
        m_light_ssbo.SetNewData(lights_vec.data(), lights_vec.size() * sizeof(Light), SSBO_BINDING::IndirectDraw3D_LIGHTS);

        //Store lengths of SSBO
        SSBO_SizesData ssbo_sizes {};
        ssbo_sizes.m_active_draw_indices_size = m_active_draw_indices.size();
        ssbo_sizes.m_mesh_transform_size      = m_mesh_transforms.size();
        ssbo_sizes.m_materials_size           = m_material_ptrs.size();
        ssbo_sizes.m_light_size               = lights_vec.size();
        m_ssbo_sizes_ubo.SetSubData(&ssbo_sizes, sizeof(SSBO_SizesData), 0);
        
        //Store mesh data
        m_vbo.SetNewData(temp_mesh_vertices);
        m_ebo.SetNewData(temp_mesh_indices);
    }

    void IndirectDraw3D_RenderPipeline::UpdateModelTransforms(const std::vector<Basic_Model*>& model_vec, const glm::mat4& view_projection) noexcept
    {
        size_t amount_meshes {0};
        for (const Basic_Model* model_ptr : model_vec)
        {
            amount_meshes += model_ptr->GetMeshVectorConstReference().size();
        }

        ENGINE_ASSERT (m_mesh_transforms.size() == amount_meshes && 
        "At IndirectDraw3D::UpdateModelTransforms(): May only be called if no models where added / removed since last call to SetSceneData().");

        m_mesh_transforms.clear();
        m_mesh_transforms.reserve(amount_meshes);

        std::vector<DrawElementsIndirectCommand> temp_draw_commands;
        temp_draw_commands.reserve(amount_meshes);
        //Offsets for draw command
        GLuint offset_indices  {0};
        GLint  offset_vertices {0};
        GLuint offset_mesh     {0};

        int culled_mesh_counter {0};

        for (const Basic_Model* model_ptr : model_vec)
        {
            const glm::mat4 model_mat   = model_ptr->GetModelMatrix();

            for (const Mesh& mesh : model_ptr->GetMeshVectorConstReference())
            {
                m_mesh_transforms.push_back(model_mat);

                const MathUtility::AABB world_space_aabb = MathUtility::AABB::CreateWorldSpaceAABB(model_mat, mesh.GetLocalAABBHalfExtents(), mesh.GetLocalCenter());

                const GLuint mesh_indices_count  = mesh.GetIndicesConstReference().size();
                const GLuint mesh_vertices_count = mesh.GetVerticesConstReference().size();

                if (MathUtility::AABBIsInFrustum(MathUtility::ExtractProjectionPlanesFromVP(view_projection), world_space_aabb))
                {
                    //Create draw command
                    DrawElementsIndirectCommand cmd;
                    cmd.count = mesh.GetIndicesConstReference().size();  // Number of indices to draw
                    cmd.instanceCount = 1;                               // One instance per model (can be >1 for instancing)
                    cmd.firstIndex    = offset_indices;                  // Offset in index buffer
                    cmd.baseVertex    = offset_vertices;                 // Offset in vertex buffer
                    cmd.baseInstance  = offset_mesh;                     // Model index for SSBO lookup

                    temp_draw_commands.push_back(cmd);
                }
                else 
                {
                    culled_mesh_counter++;
                }

                //Update offsets for next cmd
                offset_indices  += mesh_indices_count;
                offset_vertices += mesh_vertices_count;
                offset_mesh     += 1;
            }
        }

        ENGINE_PERFORMANCE_LOG_OCCURENCE("Frustum Culled Mesh: ", culled_mesh_counter);

        //Set indirect cmd data and save amount of commands
        m_indirect_command_buffer.SetSubData(temp_draw_commands.data(), temp_draw_commands.size() * sizeof(DrawElementsIndirectCommand), 0);
        m_draw_command_count = temp_draw_commands.size();

        //Set transform data
        m_mesh_transform_ssbo.SetSubData(m_mesh_transforms.data(), m_mesh_transforms.size() * sizeof(glm::mat4), 0);
    }

    void IndirectDraw3D_RenderPipeline::SetLightData(const std::vector<Light>& lights_vec) noexcept
    {
        m_light_ssbo.SetNewData(lights_vec.data(), lights_vec.size() * sizeof(Light), SSBO_BINDING::IndirectDraw3D_LIGHTS);

        SSBO_SizesData ssbo_sizes {};
        ssbo_sizes.m_active_draw_indices_size = m_active_draw_indices.size();
        ssbo_sizes.m_mesh_transform_size      = m_mesh_transforms.size();
        ssbo_sizes.m_materials_size           = m_material_ptrs.size();
        ssbo_sizes.m_light_size               = lights_vec.size();
        m_ssbo_sizes_ubo.SetSubData(&ssbo_sizes, sizeof(SSBO_SizesData), 0);
    }

    void IndirectDraw3D_RenderPipeline::SetCameraData(const glm::mat4& cam_matrix, const glm::vec3& cam_pos) noexcept
    {
        m_camera_render_data.camMatrix = cam_matrix;
        m_camera_render_data.camPos    = cam_pos;
        m_camera_ubo.SetSubData(&m_camera_render_data, sizeof(CameraRenderData), 0);
    }
}