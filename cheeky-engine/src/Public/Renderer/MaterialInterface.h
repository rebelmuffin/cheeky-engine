#pragma once

#include "Renderer/VkTypes.h"

#include <VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    class VulkanEngine;

    // Interface that is used by material types to access engine internals.
    struct MaterialEngineInterface
    {
        // internals of the engine that aren't normally exposed
        vkb::DispatchTable* device_dispatch_table;
        VmaAllocator* allocator;
        VkFormat draw_image_format;
        VkFormat depth_image_format;
        VkDescriptorSetLayout scene_data_descriptor_layout;

        // provide the engine itself as well to provide access to the engine's public interface
        VulkanEngine* engine;
    };
} // namespace Renderer