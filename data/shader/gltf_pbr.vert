#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "gltf_pbr_input.glsl"

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColour;
layout(location = 2) out vec2 outUV;

struct Vertex
{
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(push_constant) uniform constants
{
    mat4 render_matrix;
    VertexBuffer vertex_buffer;
    float opacity;
}
push_constants;

void main()
{
    // find the vertex from device address
    Vertex v = push_constants.vertex_buffer.vertices[gl_VertexIndex];

    // push output
    gl_Position = scene_data.view_projection * push_constants.render_matrix * vec4(v.position, 1.0f);
    outNormal = (push_constants.render_matrix * vec4(v.normal, 0.0f)).xyz;
    outColour = v.color.rgb * pbr_params.colour.rgb;
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}
