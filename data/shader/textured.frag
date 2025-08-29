// glsl version 4.5
#version 450

// shader input
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
// output write
layout(location = 0) out vec4 outFragColor;

// texture to access
layout(set = 1, binding = 0) uniform sampler2D displayTexture;

void main()
{
    outFragColor = inColor * vec4(texture(displayTexture, inUV).xyz, 1.0f);
}
