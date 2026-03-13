#version 460 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float line_thickness_factor;

in vec3 vertex_color[];
out vec3 frag_color;

void main()
{
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    vec2 dir = normalize(p1.xy / p1.w - p0.xy / p0.w);
    vec2 normal = vec2(-dir.y, dir.x);
    vec2 offset = normal * line_thickness_factor;

    frag_color = vertex_color[0];
    gl_Position = vec4(p0.xy + offset, p0.z, p0.w); EmitVertex();
    gl_Position = vec4(p0.xy - offset, p0.z, p0.w); EmitVertex();

    frag_color = vertex_color[1];
    gl_Position = vec4(p1.xy + offset, p1.z, p1.w); EmitVertex();
    gl_Position = vec4(p1.xy - offset, p1.z, p1.w); EmitVertex();

    EndPrimitive();
}