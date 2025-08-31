#include "Renderer/Utility/UploadRequest.h"

#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkImages.h"
#include "Renderer/VkEngine.h"
#include "Renderer/VkTypes.h"

#include <vulkan/vulkan_core.h>

#include <cstddef>

namespace Renderer::Utils
{
    MeshUploadRequest::MeshUploadRequest(
        size_t vertex_buffer_size,
        size_t index_buffer_size,
        const GPUMeshBuffers& target_mesh,
        const BufferHandle& staging_buffer,
        UploadType upload_type,
        std::string_view debug_name
    ) :
        m_vertex_buffer_size(vertex_buffer_size),
        m_index_buffer_size(index_buffer_size),
        m_target_mesh(target_mesh),
        m_staging_buffer(staging_buffer),
        m_upload_type(upload_type),
        m_debug_name(debug_name)
    {
    }

    UploadExecutionResult MeshUploadRequest::ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd)
    {
        VkBufferCopy vertex_copy{};
        vertex_copy.dstOffset = 0;
        vertex_copy.srcOffset = 0;
        vertex_copy.size = m_vertex_buffer_size;

        VkBufferCopy index_copy{};
        index_copy.dstOffset = 0;
        index_copy.srcOffset = m_vertex_buffer_size;
        index_copy.size = m_index_buffer_size;

        engine.DeviceDispatchTable().cmdCopyBuffer(
            cmd, m_staging_buffer->buffer, m_target_mesh.vertex_buffer->buffer, 1, &vertex_copy
        );
        engine.DeviceDispatchTable().cmdCopyBuffer(
            cmd, m_staging_buffer->buffer, m_target_mesh.index_buffer->buffer, 1, &index_copy
        );

        return UploadExecutionResult::Success;
    }

    void MeshUploadRequest::DestroyResources(VulkanEngine&)
    {
        // reference counted handles should be enough.
    }

    BufferUploadRequest::BufferUploadRequest(
        size_t buffer_size,
        BufferHandle staging_buffer,
        BufferHandle target_buffer,
        UploadType upload_type,
        size_t src_offset,
        size_t dst_offset,
        std::string_view debug_name
    ) :
        m_buffer_size(buffer_size),
        m_src_offset(src_offset),
        m_dst_offset(dst_offset),
        m_staging_buffer(staging_buffer),
        m_target_buffer(target_buffer),
        m_upload_type(upload_type),
        m_debug_name(debug_name)
    {
    }

    UploadExecutionResult BufferUploadRequest::ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd)
    {
        VkBufferCopy copy{};
        copy.dstOffset = m_dst_offset;
        copy.srcOffset = m_src_offset;
        copy.size = m_buffer_size;

        engine.DeviceDispatchTable().cmdCopyBuffer(
            cmd, m_staging_buffer->buffer, m_target_buffer->buffer, 1, &copy
        );

        return UploadExecutionResult::Success;
    }

    void BufferUploadRequest::DestroyResources(VulkanEngine&)
    {
        // no need for explicity destroy. the reference counted handles should do the trick
    }

    ImageUploadRequest::ImageUploadRequest(
        VkExtent3D image_extent,
        ImageHandle staging_image,
        ImageHandle target_image,
        UploadType upload_type,
        VkOffset3D src_offset,
        VkOffset3D dst_offset,
        std::string_view debug_name
    ) :
        m_image_extent(image_extent),
        m_src_offset(src_offset),
        m_dst_offset(dst_offset),
        m_staging_image(staging_image),
        m_target_image(target_image),
        m_upload_type(upload_type),
        m_debug_name(debug_name)
    {
    }

    UploadExecutionResult ImageUploadRequest::ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd)
    {
        VkImageCopy2 copy{};
        copy.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
        copy.dstOffset = m_dst_offset;
        copy.srcOffset = m_src_offset;
        copy.extent = m_image_extent;
        copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.srcSubresource.layerCount = 1;
        copy.dstSubresource.layerCount = 1;

        // we need to first transition the images
        Utils::TransitionImage(
            &engine.DeviceDispatchTable(),
            cmd,
            m_staging_image->image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );
        Utils::TransitionImage(
            &engine.DeviceDispatchTable(),
            cmd,
            m_target_image->image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        VkCopyImageInfo2 copy_image_info{};
        copy_image_info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
        copy_image_info.dstImage = m_target_image->image;
        copy_image_info.srcImage = m_staging_image->image;
        copy_image_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_image_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copy_image_info.regionCount = 1;
        copy_image_info.pRegions = &copy;

        engine.DeviceDispatchTable().cmdCopyImage2(cmd, &copy_image_info);

        return UploadExecutionResult::Success;
    }

    void ImageUploadRequest::DestroyResources(VulkanEngine&)
    {
        // no need for explicity destroy. the reference counted handles should do the trick
    }
} // namespace Renderer::Utils