#version 460 core

in vec3 vertex_color;

out vec4 FragColor;

void main()
{
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;

    float edge = 0.05;
    float alpha = 1.0 - smoothstep(1.0 - edge, 1.0, dist);

    if (alpha < 0.01) discard;

    FragColor = vec4(vertex_color, alpha);
}