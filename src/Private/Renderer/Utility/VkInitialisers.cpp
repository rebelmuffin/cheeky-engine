#include "Renderer/Utility/VkInitialisers.h"

namespace Renderer::Utils
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queue_family_index, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo commandPoolInfo{};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = nullptr;
        commandPoolInfo.flags = flags;
        commandPoolInfo.queueFamilyIndex = queue_family_index;
        return commandPoolInfo;
    }

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool command_pool, uint32_t count)
    {
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = command_pool;
        cmdAllocInfo.commandBufferCount = count;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return cmdAllocInfo;
    }

    VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags)
    {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;
        return info;
    }

    VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags)
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;
        return info;
    }

    VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags)
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;
        info.pInheritanceInfo = nullptr;
        info.flags = flags;
        return info;
    }

    VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore)
    {
        VkSemaphoreSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        info.pNext = nullptr;
        info.semaphore = semaphore;
        info.stageMask = stage_mask;
        info.deviceIndex = 0;
        info.value = 1;
        return info;
    }

    VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd)
    {
        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext = nullptr;
        info.commandBuffer = cmd;
        info.deviceMask = 0;
        return info;
    }

    VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_semaphore_info,
                             VkSemaphoreSubmitInfo* wait_semaphore_info)
    {
        VkSubmitInfo2 info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext = nullptr;
        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = cmd;
        info.waitSemaphoreInfoCount = wait_semaphore_info == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = wait_semaphore_info;
        info.signalSemaphoreInfoCount = signal_semaphore_info == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signal_semaphore_info;
        return info;
    }

    VkPresentInfoKHR PresentInfo(VkSwapchainKHR* swapchain, VkSemaphore* wait_semaphore,
                                 uint32_t* swapchain_image_index)
    {
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = nullptr;
        info.pImageIndices = swapchain_image_index;
        info.swapchainCount = 1;
        info.pSwapchains = swapchain;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = wait_semaphore;
        return info;
    }

    VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent)
    {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.imageType = VK_IMAGE_TYPE_2D;

        info.format = format;
        info.usage = flags;
        info.extent = extent;

        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT; // no MSAA
        info.tiling = VK_IMAGE_TILING_OPTIMAL;

        return info;
    }

    VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags)
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        info.format = format;
        info.image = image;
        info.subresourceRange.aspectMask = aspect_flags;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;

        return info;
    }

    VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout)
    {
        VkRenderingAttachmentInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.pNext = nullptr;
        info.imageView = view;
        info.imageLayout = layout;
        info.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (clear)
        {
            info.clearValue = *clear;
        }

        return info;
    }

    VkRenderingInfo RenderingInfo(VkRenderingAttachmentInfo* color_attachment_info,
                                  VkRenderingAttachmentInfo* depth_attachment_info, VkExtent2D draw_extent)
    {
        VkRenderingInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        info.pNext = nullptr;
        info.layerCount = 1;
        info.colorAttachmentCount = 1;
        info.pColorAttachments = color_attachment_info;
        info.pDepthAttachment = depth_attachment_info;
        info.renderArea = {{0, 0}, {draw_extent.width, draw_extent.height}};

        return info;
    }

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo(const char* name, VkShaderModule shader,
                                                          VkShaderStageFlagBits stage)
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.pName = name;
        info.stage = stage;
        info.module = shader;

        return info;
    }

} // namespace Renderer::Utils
