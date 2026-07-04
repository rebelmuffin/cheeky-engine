#pragma once

#include "Renderer/ResourceStorage.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#define VK_CHECK(x)                                                                                          \
    do                                                                                                       \
    {                                                                                                        \
        VkResult err = x;                                                                                    \
        if (err)                                                                                             \
        {                                                                                                    \
            std::cout << "Detected Vulkan error: " << string_VkResult(err) << std::endl;                     \
            abort();                                                                                         \
        }                                                                                                    \
    } while (0)

namespace Renderer
{
    struct AllocatedImage
    {
        VkImage image;
        VkImageView image_view;
        VmaAllocation allocation;
        VkExtent3D image_extent;
        VkFormat image_format;
    };

    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;
    };

    struct Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 colour;
    };

    // template specialisations for resource storages
    void DestroyImage(VulkanEngine& engine, const AllocatedImage& image);
    void DestroyBuffer(VulkanEngine& engine, const AllocatedBuffer& buffer);

    template <>
    inline void ResourceStorage<AllocatedImage>::DestroyResource(
        VulkanEngine& engine, const AllocatedImage& image
    )
    {
        DestroyImage(engine, image);
    }

    template <>
    inline void ResourceStorage<AllocatedBuffer>::DestroyResource(
        VulkanEngine& engine, const AllocatedBuffer& buffer
    )
    {
        DestroyBuffer(engine, buffer);
    }

    using ImageHandle = ReferenceCountedHandle<AllocatedImage>;
    using BufferHandle = ReferenceCountedHandle<AllocatedBuffer>;

    struct GPUMeshBuffers
    {
        BufferHandle index_buffer;
        BufferHandle vertex_buffer;
        VkDeviceAddress vertex_buffer_address;
    };

    struct GPUDrawPushConstants
    {
        glm::mat4 world_matrix;
        VkDeviceAddress vertex_buffer_address;
        float opacity;
    };

    struct GPUSceneData
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 view_projection;
        glm::vec4 ambient_colour = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        glm::vec4 light_direction = glm::vec4(0.34f, 0.33f, 0.33f, 0.0f);
        glm::vec4 light_colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    };
} // namespace Renderer