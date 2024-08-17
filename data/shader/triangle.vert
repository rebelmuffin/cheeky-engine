#version 450

layout (location = 0) out vec3 outColour;

void main()
{
    const vec3 pos[3] = vec3[](
    vec3(1.0f, 1.0f, 0.0f),
    vec3(-1.0f, 1.0f, 0.0f),
    vec3(0.0f, -1.0f, 0.0f)
    );

    const vec3 colours[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
    );

    gl_Position = vec4(pos[gl_VertexIndex], 1.0);
    outColour = colours[gl_VertexIndex];
}
