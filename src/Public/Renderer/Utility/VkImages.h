#pragma once

#include <vulkan/vulkan.h>

namespace vkb
{
    struct DispatchTable;
}

namespace Renderer::Utils
{
    void TransitionImage(
        vkb::DispatchTable* device_dispatch,
        VkCommandBuffer cmd,
        VkImage image,
        VkImageLayout current_layout,
        VkImageLayout target_layout
    );

    void CopyImageToImage(
        vkb::DispatchTable* device_dispatch,
        VkCommandBuffer cmd,
        VkImage source_image,
        VkImage dest_image,
        VkExtent2D source_size,
        VkExtent2D dest_size
    );

    VkImageSubresourceRange SubresourceRange(VkImageAspectFlags aspect_mask);
} // namespace Renderer::Utils