#pragma once

#include "Renderer/Material.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    // Represents all the required info to render an object using cmdDrawIndexed
    struct RenderObject
    {
        uint32_t index_count;
        uint32_t first_index;
        VkBuffer index_buffer;

        MaterialInstance* material;
        glm::mat4 transform;
        VkDeviceAddress vertex_buffer_address;
    };
} // namespace Renderer