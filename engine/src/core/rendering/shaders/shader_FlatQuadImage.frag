#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D tex_bindless_handle;

void main() 
{
    FragColor = texture(tex_bindless_handle, texCoord);
}