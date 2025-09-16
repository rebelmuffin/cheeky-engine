#pragma once

#include <glm/ext/matrix_float4x4.hpp>
namespace Renderer
{
    struct Camera
    {
        glm::mat4 view{ 1.0f };
        glm::mat4 projection{ 1.0f };
    };
} // namespace Renderer