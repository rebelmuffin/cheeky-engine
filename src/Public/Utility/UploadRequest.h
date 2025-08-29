#pragma once

#include "VkLoader.h"
#include "VkTypes.h"

class VulkanEngine;

namespace Utils
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
        /// MeshUploadRequest will take ownership of the staging buffer and destroy it after the upload is complete.
        MeshUploadRequest(size_t vertex_buffer_size, size_t index_buffer_size, const GPUMeshBuffers& target_mesh,
                          const AllocatedBuffer& staging_buffer, UploadType upload_type,
                          std::string_view debug_name = "unnamed_mesh_upload");
        virtual ~MeshUploadRequest() = default;

        UploadExecutionResult ExecuteUpload(VulkanEngine& engine, VkCommandBuffer cmd) override;
        void DestroyResources(VulkanEngine& engine) override;

        std::string_view DebugName() const override
        {
            return m_debug_name;
        }
        UploadType GetUploadType() const override
        {
            return m_upload_type;
        }

      private:
        size_t m_vertex_buffer_size;
        size_t m_index_buffer_size;
        GPUMeshBuffers m_target_mesh;
        AllocatedBuffer m_staging_buffer;
        UploadType m_upload_type;
        std::string m_debug_name;
    };
} // namespace Utils