#include "Utility/VkImages.h"
#include "VkBootstrapDispatch.h"

namespace Utils
{
    void TransitionImage(vkb::DispatchTable* device_dispatch, VkCommandBuffer cmd, VkImage image,
                         VkImageLayout current_layout, VkImageLayout target_layout)
    {
        VkImageMemoryBarrier2 imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = current_layout;
        imageBarrier.newLayout = target_layout;

        VkImageAspectFlags aspectMask = target_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                                            : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = SubresourceRange(aspectMask);
        imageBarrier.image = image;

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;

        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        device_dispatch->cmdPipelineBarrier2(cmd, &depInfo);
    }

    VkImageSubresourceRange SubresourceRange(VkImageAspectFlags aspect_mask)
    {
        VkImageSubresourceRange subImage{};
        subImage.aspectMask = aspect_mask;
        subImage.baseMipLevel = 0;
        subImage.levelCount = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

        return subImage;
    }
} // namespace Utils