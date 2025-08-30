#include "Renderer/Utility/VkImages.h"

#include <VkBootstrapDispatch.h>

namespace Renderer::Utils
{
    void TransitionImage(
        vkb::DispatchTable* device_dispatch,
        VkCommandBuffer cmd,
        VkImage image,
        VkImageLayout current_layout,
        VkImageLayout target_layout
    )
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

    void CopyImageToImage(
        vkb::DispatchTable* device_dispatch,
        VkCommandBuffer cmd,
        VkImage source_image,
        VkImage dest_image,
        VkExtent2D source_size,
        VkExtent2D dest_size
    )
    {
        VkImageBlit2 blit_region{};
        blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
        blit_region.pNext = nullptr;

        blit_region.srcOffsets[1].x = int32_t(source_size.width);
        blit_region.srcOffsets[1].y = int32_t(source_size.height);
        blit_region.srcOffsets[1].z = 1;
        blit_region.dstOffsets[1].x = int32_t(dest_size.width);
        blit_region.dstOffsets[1].y = int32_t(dest_size.height);
        blit_region.dstOffsets[1].z = 1;

        blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_region.srcSubresource.baseArrayLayer = 0;
        blit_region.srcSubresource.mipLevel = 0;
        blit_region.srcSubresource.layerCount = 1;

        blit_region.dstSubresource = blit_region.srcSubresource;

        VkBlitImageInfo2 blit_info{};
        blit_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
        blit_info.pNext = nullptr;

        blit_info.srcImage = source_image;
        blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blit_info.dstImage = dest_image;
        blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blit_info.filter = VK_FILTER_NEAREST; // NEAREST!!!
        blit_info.regionCount = 1;
        blit_info.pRegions = &blit_region;

        device_dispatch->cmdBlitImage2(cmd, &blit_info);
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
} // namespace Renderer::Utils