#version 460 core
#extension GL_ARB_bindless_texture : require
//#extension GL_ARB_gpu_shader_int64 : require

out vec4 FragColor;

in vec3 view_direction;
in vec3 vertex_world_pos;
in vec3 normal;
in vec2 tex_uv;
in flat uint calculated_model_index;

struct Material 
{
    vec3  m_base_color_factor; 
    float m_metallic_factor;

    vec3  m_emissive_factor;
    float m_roughness_factor;

    uvec2 m_base_color_tex_handle;
    uvec2 m_metallic_roughness_tex_handle;
    uvec2 m_normal_tex_handle;
    uvec2 m_occlusion_tex_handle;
    uvec2 m_emissive_tex_handle;

    float[2] _padding; 
};

#define LIGHT_MODE_OFF          0
#define LIGHT_MODE_POINT_LIGHT  1
#define LIGHT_MODE_SPOT_LIGHT   2
#define LIGHT_MODE_DIRECT_LIGHT 3

struct Light 
{
    vec3 light_position; float light_intensity;  // 16 bytes
    vec3 light_color;    uint  light_mode;       // 16 bytes
};

/////////////////////////////////////////////// 
//--------- SSBOs
//////////////////////////////////////////////// 
layout(std430, binding = 1) buffer MaterialsBuffer 
{
    Material materials[];
};

layout(std430, binding = 2) buffer LightingDataBuffer 
{
    Light lights[];
};

/////////////////////////////////////////////// 
//--------- UBOs
//////////////////////////////////////////////// 
layout(std140, binding = 1) uniform SSBO_sizesBuffer 
{
    uint active_draw_indices_ssbo_size;
    uint mesh_transform_ssbo_size;
    uint materials_ssbo_size;
    uint light_ssbo_size;
};

/////////////////////////////////////////////// 
//--------- Helper Functions
////////////////////////////////////////////////
vec4 SampleTexture(uvec2 handle, vec2 uv, vec4 fallback_tex) 
{
    if (handle.x == 0 && handle.y == 0) return fallback_tex;
    sampler2D tex = sampler2D(handle);
    return texture(tex, uv);
}

vec3 SampleNormal(uvec2 handle, vec2 uv, vec3 fallback_normal) 
{
    if (handle.x == 0 && handle.y == 0) return fallback_normal;
    sampler2D tex = sampler2D(handle);
    vec3 n = texture(tex, uv).rgb;
    return normalize(n * 2.0 - 1.0);
}

vec3 ComputeLight(Light light, vec3 N, vec3 V, vec3 fragPos, vec3 baseColor) 
{
    vec3 L, R;
    float diff, spec, attenuation;
    vec3 lightContrib = vec3(0.0);

    if (light.light_mode == LIGHT_MODE_DIRECT_LIGHT) 
    {
        float strength_scale = 1 / 200.0f;

        L = normalize(vec3(1.0, 1.0, 0.0)); 
        
        diff = max(dot(N, L), 0.0);
        R = reflect(-L, N);
        spec = pow(max(dot(V, R), 0.0), 32.0);
        
        lightContrib = (diff + spec) * light.light_color * light.light_intensity * strength_scale;
    }
    
    else if (light.light_mode == LIGHT_MODE_POINT_LIGHT) 
    {
        float strength_scale = 50;
        
        L = light.light_position - fragPos;
        float dist = length(L);
        L = normalize(L);
        
        attenuation = light.light_intensity / (0.1 * dist * dist + 0.01 * dist + 1.0);
        
        diff = max(dot(N, L), 0.0);
        R = reflect(-L, N);
        spec = pow(max(dot(V, R), 0.0), 32.0);
        
        lightContrib = (diff + spec) * attenuation * light.light_color * strength_scale;
    }
    
    else if (light.light_mode == LIGHT_MODE_SPOT_LIGHT) 
    {
        float strength_scale = 50;

        L = light.light_position - fragPos;
        float dist = length(L);
        L = normalize(L);

        vec3 spotDir = normalize(vec3(0.0, -1.0, 0.0)); 

        float cosInner = 0.976; 
        float cosOuter = 0.953; 
        
        float theta = dot(-L, spotDir); 
        float epsilon = cosInner - cosOuter;
        
        float spotIntensity = clamp((theta - cosOuter) / epsilon, 0.0, 1.0);

        attenuation = light.light_intensity / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        diff = max(dot(N, L), 0.0);
        R = reflect(-L, N);
        spec = pow(max(dot(V, R), 0.0), 32.0);
        
        lightContrib = (diff + spec) * attenuation * spotIntensity * light.light_color * strength_scale;
    }

    return lightContrib * baseColor;
}

/////////////////////////////////////////////// 
//--------- Main Fragment Shader
/////////////////////////////////////////////// 
void main() 
{
    Material mat = materials[calculated_model_index];

    vec4 baseCol  = SampleTexture(mat.m_base_color_tex_handle, tex_uv, vec4(mat.m_base_color_factor,1.0));
    vec3 N        = SampleNormal(mat.m_normal_tex_handle,      tex_uv, normal);
    vec4 emissive = SampleTexture(mat.m_emissive_tex_handle,   tex_uv, vec4(mat.m_emissive_factor,1.0));

    vec3 V = normalize(view_direction);

    vec3 ambient = baseCol.rgb * 0.1; 
    vec3 totalLight = vec3(0.0);

    for (uint i = 0; i < light_ssbo_size; i++) 
    {
        if (lights[i].light_intensity < 0.001) continue;
        totalLight += ComputeLight(lights[i], N, V, vertex_world_pos, baseCol.rgb);
    }

    vec3 finalColor = ambient + totalLight + emissive.rgb;

    FragColor = vec4(finalColor, baseCol.a);
}