#version 460 core

/////////////////////////////////////////////// 
//--------- Layout in variables
//////////////////////////////////////////////// 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexUV;

/////////////////////////////////////////////// 
//--------- SSBOs
//////////////////////////////////////////////// 
layout(std430, binding = 0) buffer TransformBuffer 
{
    mat4 model_matrices[];
};

layout(std430, binding = 3) buffer ActiveIndicesBuffer 
{
    uint active_draw_indices[];
};

/////////////////////////////////////////////// 
//--------- UBOs
//////////////////////////////////////////////// 
layout(std140, binding = 2) uniform CameraData 
{
    mat4 cam_matrix;
    vec3 cam_pos; float padding;
};

out vec3 view_direction;
out vec3 vertex_world_pos;
out vec3 normal;
out vec2 tex_uv;
out uint calculated_model_index;

void main()
{
    calculated_model_index = active_draw_indices[gl_BaseInstance + gl_InstanceID];
    mat4 model             = model_matrices[calculated_model_index];
    vertex_world_pos       = vec3(model * vec4(aPos, 1.0f));
    view_direction         = normalize(cam_pos - vertex_world_pos);
    normal                 = normalize(mat3(transpose(inverse(model))) * aNormal);
    tex_uv                 = aTexUV;
    gl_Position            = cam_matrix * vec4(vertex_world_pos, 1.0);
}