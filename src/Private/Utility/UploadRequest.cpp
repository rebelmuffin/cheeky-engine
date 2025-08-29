#include "Utility/UploadRequest.h"

#include "VkEngine.h"

#include <cstddef>

namespace Utils
{
    MeshUploadRequest::MeshUploadRequest(size_t vertex_buffer_size, size_t index_buffer_size,
                                         const GPUMeshBuffers& target_mesh, const AllocatedBuffer& staging_buffer,
                                         UploadType upload_type, std::string_view debug_name)
        : m_vertex_buffer_size(vertex_buffer_size), m_index_buffer_size(index_buffer_size), m_target_mesh(target_mesh),
          m_staging_buffer(staging_buffer), m_upload_type(upload_type), m_debug_name(debug_name)
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

        engine.DeviceDispatchTable().cmdCopyBuffer(cmd, m_staging_buffer.buffer, m_target_mesh.vertex_buffer.buffer, 1,
                                                   &vertex_copy);
        engine.DeviceDispatchTable().cmdCopyBuffer(cmd, m_staging_buffer.buffer, m_target_mesh.index_buffer.buffer, 1,
                                                   &index_copy);

        return UploadExecutionResult::Success;
    }

    void MeshUploadRequest::DestroyResources(VulkanEngine& engine)
    {
        engine.DestroyBuffer(m_staging_buffer);
    }
} // namespace Utils