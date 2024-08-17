#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 colour;

layout (location = 0) out vec3 outColour;

layout (push_constant) uniform constants
{
    vec4 data;
    mat4 render_matrix;
} PushConstants;

void main() {
    gl_Position = PushConstants.render_matrix * vec4(position, 1.0);
    outColour = colour;
}
