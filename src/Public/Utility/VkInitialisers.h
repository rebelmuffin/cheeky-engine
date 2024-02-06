#pragma once

#include "VkTypes.h"

namespace Utils
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags);
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool command_pool, uint32_t count);

    VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags);
    VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags);

    VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);
    VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
    VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
    VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_semaphore_info,
                             VkSemaphoreSubmitInfo* wait_semaphore_info);
    VkPresentInfoKHR PresentInfo(VkSwapchainKHR* swapchain, VkSemaphore* wait_semaphore,
                                 uint32_t* swapchain_image_index);
} // namespace Utils