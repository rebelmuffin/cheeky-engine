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

void main()
{
    // find the vertex from device address
    Vertex v = PushConstants.vertex_buffer.vertices[gl_VertexIndex];

    // push output
    gl_Position = scene_data.view_projection * PushConstants.render_matrix * vec4(v.position, 1.0f);
    // very basic directional lighting
    float light_intensity = max(dot(normalize(v.normal), normalize(-scene_data.light_direction.xyz)), 0.0);
    vec3 lit_colour = v.color.rgb * (scene_data.ambient_colour.rgb + scene_data.light_colour.rgb * light_intensity);
    outColour = vec4(lit_colour, PushConstants.opacity);
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}
