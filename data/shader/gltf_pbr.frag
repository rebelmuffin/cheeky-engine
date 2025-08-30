#version 450

#extension GL_GOOGLE_include_directive : require

#include "gltf_pbr_input.glsl"

// shader input
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inUV;
// output write
layout(location = 0) out vec4 outFragColor;

void main()
{
    float light_intensity = max(dot(scene_data.light_direction.xyz, inNormal), 0.1f); // minimum 0.1f intensity

    vec3 base_colour = inColour * texture(colour_texture, inUV).xyz;
    vec3 ambient_colour = base_colour * scene_data.ambient_colour.xyz;
    vec3 colour_with_light = base_colour * light_intensity * scene_data.light_colour.xyz;

    outFragColor = vec4(colour_with_light + ambient_colour, 1.0f); // no transparency yet
}
