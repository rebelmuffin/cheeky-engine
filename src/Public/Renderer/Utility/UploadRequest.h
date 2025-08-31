#pragma once

#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    class VulkanEngine;
}

namespace Renderer::Utils
{
    enum class UploadExecutionResult
    {
        Success,
        Failed,
        RetryNextFrame
    };

    enum class UploadType
    {
        Immediate,
        Deferred
    };

    class IUploadRequest
    {
      public:
        virtual ~IUploadRequest() = default;

        /// This is called during either an immediate upload, or during the normal frame upload phase.
        /// The command buffer is already begun and ended outside of this call.
        virtual UploadExecutionResult ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd) = 0;

        /// If your upload request owns any resources that need to be destroyed, do it here.
        /// This is called after the upload has been executed, when it is safe to destroy any GPU resources.
        virtual void DestroyResources(VulkanEngine& engine) = 0;

        virtual std::string_view DebugName() const = 0;

        virtual UploadType GetUploadType() const = 0;
    };

    class MeshUploadRequest : public IUploadRequest
    {
      public:
        /// MeshUploadRequest will take ownership of the staging buffer and destroy it after the upload is
        /// complete.
        MeshUploadRequest(
            size_t vertex_buffer_size,
            size_t index_buffer_size,
            const GPUMeshBuffers& target_mesh,
            const BufferHandle& staging_buffer,
            UploadType upload_type,
            std::string_view debug_name = "unnamed_mesh_upload"
        );
        virtual ~MeshUploadRequest() = default;

        UploadExecutionResult ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd) override;
        void DestroyResources(VulkanEngine& engine) override;

        std::string_view DebugName() const override { return m_debug_name; }
        UploadType GetUploadType() const override { return m_upload_type; }

      private:
        size_t m_vertex_buffer_size;
        size_t m_index_buffer_size;
        GPUMeshBuffers m_target_mesh;
        BufferHandle m_staging_buffer;
        UploadType m_upload_type;
        std::string m_debug_name;
    };

    class BufferUploadRequest : public IUploadRequest
    {
      public:
        /// BufferUploadRequest will take ownership of the staging buffer and destroy it after the upload is
        /// complete.
        BufferUploadRequest(
            size_t buffer_size,
            BufferHandle staging_buffer,
            BufferHandle target_buffer,
            UploadType upload_type,
            size_t src_offset = 0,
            size_t dst_offset = 0,
            std::string_view debug_name = "unnamed_buffer_upload"
        );
        virtual ~BufferUploadRequest() = default;

        UploadExecutionResult ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd) override;
        void DestroyResources(VulkanEngine& engine) override;

        std::string_view DebugName() const override { return m_debug_name; }
        UploadType GetUploadType() const override { return m_upload_type; }

      private:
        size_t m_buffer_size;
        size_t m_src_offset;
        size_t m_dst_offset;
        BufferHandle m_staging_buffer;
        BufferHandle m_target_buffer;
        UploadType m_upload_type;
        std::string m_debug_name;
    };

    class ImageUploadRequest : public IUploadRequest
    {
      public:
        /// ImageUploadRequest will take ownership of the staging image and destroy it after the upload is
        /// complete.
        ImageUploadRequest(
            VkExtent3D image_extent,
            ImageHandle staging_image,
            ImageHandle target_image,
            UploadType upload_type,
            VkOffset3D src_offset = {},
            VkOffset3D dst_offset = {},
            std::string_view debug_name = "unnamed_image_upload"
        );
        virtual ~ImageUploadRequest() = default;

        UploadExecutionResult ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd) override;
        void DestroyResources(VulkanEngine& engine) override;

        std::string_view DebugName() const override { return m_debug_name; }
        UploadType GetUploadType() const override { return m_upload_type; }

      private:
        VkExtent3D m_image_extent;
        VkOffset3D m_src_offset;
        VkOffset3D m_dst_offset;
        ImageHandle m_staging_image;
        ImageHandle m_target_image;
        UploadType m_upload_type;
        std::string m_debug_name;
    };
} // namespace Renderer::Utils