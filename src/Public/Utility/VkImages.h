#pragma once

#include <vulkan/vulkan.h>

namespace vkb
{
    struct DispatchTable;
}

namespace Utils
{
    void TransitionImage(vkb::DispatchTable* device_dispatch, VkCommandBuffer cmd, VkImage image,
                         VkImageLayout current_layout, VkImageLayout target_layout);
    VkImageSubresourceRange SubresourceRange(VkImageAspectFlags aspect_mask);
} // namespace Utils