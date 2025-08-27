#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec4 outColour;
layout(location = 1) out vec2 outUV;

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
PushConstants;

void main()
{
    // find the vertex from device address
    Vertex v = PushConstants.vertex_buffer.vertices[gl_VertexIndex];

    // push output
    gl_Position = PushConstants.render_matrix * vec4(v.position, 1.0f);
    outColour = vec4(v.color.rgb, PushConstants.opacity);
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}
