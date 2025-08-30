layout(set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    vec4 ambient_colour;
    vec4 light_direction;
    vec4 light_colour;
}
scene_data;

layout(set = 1, binding = 0) uniform MaterialParams
{
    vec4 colour;
    vec4 metal_roughness;
}
pbr_params;

layout(set = 1, binding = 1) uniform sampler2D colour_texture;
layout(set = 1, binding = 2) uniform sampler2D metal_roughness_texture;