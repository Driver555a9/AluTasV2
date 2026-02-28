#version 460 core

out vec4 FragColor;

in vec3 vertex_world_pos;
in vec3 normal;

uniform vec4 ghost_color_uniform;

/////////////////////////////////////////////// 
//--------- Main Fragment Shader
/////////////////////////////////////////////// 
void main() 
{
    FragColor = ghost_color_uniform;
}