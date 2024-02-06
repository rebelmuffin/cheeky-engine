#pragma once

#include <vulkan/vulkan.h>

namespace Utils
{
    void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout target_layout);
    VkImageSubresourceRange SubresourceRange(VkImageAspectFlags aspect_mask);
} // namespace Utils