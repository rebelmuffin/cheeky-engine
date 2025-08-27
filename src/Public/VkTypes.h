#pragma once

#include <array>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(x)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult err = x;                                                                                              \
        if (err)                                                                                                       \
        {                                                                                                              \
            std::cout << "Detected Vulkan error: " << static_cast<uint64_t>(err) << std::endl;                         \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

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

struct GPUMeshBuffers
{
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

struct GPUDrawPushConstants
{
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer_address;
};