#version 450

#extension GL_GOOGLE_include_directive : require

// shader input
layout(location = 0) in vec3 inColour;
// output write
layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(inColour, 1);
}
