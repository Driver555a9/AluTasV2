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

out vec3 vertex_world_pos;
out vec3 normal;

void main()
{
    mat4 model        = model_matrices[active_draw_indices[gl_BaseInstance + gl_InstanceID]];
    vertex_world_pos  = vec3(model * vec4(aPos, 1.0f));
    normal            = normalize(mat3(transpose(inverse(model))) * aNormal);
    gl_Position       = cam_matrix * vec4(vertex_world_pos, 1.0);
}