#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

uniform mat4 camera_matrix_uniform;

out vec3 vertex_color;

void main()
{
    gl_Position  = camera_matrix_uniform * vec4(in_position, 1.0);
    vertex_color = in_color;
}