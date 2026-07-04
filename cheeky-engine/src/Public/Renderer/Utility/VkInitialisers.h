#pragma once

#include "Renderer/VkTypes.h"
#include <vulkan/vulkan_core.h>

namespace Renderer::Utils
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo(
        uint32_t queue_family_index, VkCommandPoolCreateFlags flags
    );
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool command_pool, uint32_t count);

    VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags);
    VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags);

    VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
    VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
    VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
    VkSubmitInfo2 SubmitInfo(
        VkCommandBufferSubmitInfo* cmd,
        VkSemaphoreSubmitInfo* signal_semaphore_info,
        VkSemaphoreSubmitInfo* wait_semaphore_info
    );
    VkPresentInfoKHR PresentInfo(
        VkSwapchainKHR* swapchain, VkSemaphore* wait_semaphore, uint32_t* swapchain_image_index
    );

    VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extents);
    VkImageViewCreateInfo ImageViewCreateInfo(
        VkFormat format, VkImage image, VkImageAspectFlags aspect_flags
    );

    VkRenderingAttachmentInfo AttachmentInfo(
        VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    VkRenderingInfo RenderingInfo(
        VkRenderingAttachmentInfo* color_attachment_info,
        VkRenderingAttachmentInfo* depth_attachment_info,
        VkExtent2D draw_extent
    );
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo(
        const char* name, VkShaderModule shader, VkShaderStageFlagBits stage
    );
} // namespace Renderer::Utils