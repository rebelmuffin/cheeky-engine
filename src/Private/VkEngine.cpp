#include "VkEngine.h"
#include "ThirdParty/ImGUI.h"
#include "ThirdParty/VkMemAlloc.h"
#include "Utility/VkImages.h"
#include "Utility/VkInitialisers.h"
#include "Utility/VkLoader.h"
#include "Utility/VkPipelines.h"
#include "VkTypes.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vulkan/vulkan_core.h>

#define VK_DEVICE_CALL(device, function, ...)                                                                          \
    reinterpret_cast<PFN_##function>(m_get_device_proc_addr(device, #function))(device, __VA_ARGS__);

#define VK_INSTANCE_CALL(instance, function, ...)                                                                      \
    reinterpret_cast<PFN_##function>(m_get_instance_proc_addr(instance, #function))(instance, __VA_ARGS__);

VulkanEngine::VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window, float backbuffer_scale,
                           bool use_validation_layers, bool immediate_uploads)
    : m_backbuffer_scale(backbuffer_scale), m_window_extent({window_width, window_height}), m_window(window),
      m_use_validation_layers(use_validation_layers), m_immediate_uploads_enabled(immediate_uploads)
{
}

namespace
{
    bool CreateComputeEffect(const char* name, const char* shader_path, vkb::DispatchTable& device_dispatch,
                             VkPipelineLayout layout, Utils::DeletionQueue& deletion_queue, ComputeEffect* out_effect)
    {
        VkShaderModule shader_module{};
        if (Utils::LoadShaderModule(device_dispatch, shader_path, &shader_module) == false)
        {
            return false;
        }

        ComputeEffect effect{};
        effect.name = name;
        effect.path = shader_path;
        effect.layout = layout;

        VkPipelineShaderStageCreateInfo stage_info{};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.pNext = nullptr;
        stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage_info.module = shader_module;
        stage_info.pName = "main";

        VkComputePipelineCreateInfo compute_pipeline_info{};
        compute_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_info.pNext = nullptr;
        compute_pipeline_info.layout = layout;
        compute_pipeline_info.stage = stage_info;

        VK_CHECK(device_dispatch.createComputePipelines(VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr,
                                                        &effect.pipeline));

        // Clean up
        device_dispatch.destroyShaderModule(shader_module, nullptr);
        deletion_queue.PushFunction("pipelines", [device_dispatch, pipeline = effect.pipeline]() {
            device_dispatch.destroyPipeline(pipeline, nullptr);
        });

        *out_effect = effect;
        return true;
    }
} // namespace

bool VulkanEngine::Init()
{
    if (InitVulkan() == false)
    {
        return false;
    }

    InitAllocator();
    ResetSwapchain();
    InitCommands();
    InitSyncStructures();
    InitDescriptors();

    if (InitPipelines() == false)
    {
        return false;
    }

    // Imgui
    InitImgui();

    InitDefaultData();

    return true;
}

void VulkanEngine::Cleanup()
{
    m_device_dispatch.deviceWaitIdle();

    m_deletion_queue.Flush();

    VK_DEVICE_CALL(m_device, vkDestroyDevice, nullptr);
    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
    VK_INSTANCE_CALL(m_instance, vkDestroyInstance, nullptr);
    is_initialised = false;
}

void VulkanEngine::Draw([[maybe_unused]] double delta_ms)
{
    constexpr uint64_t one_second_ns = 1'000'000'000;
    VK_CHECK(m_device_dispatch.waitForFences(1, &GetCurrentFrame().render_fence, true, one_second_ns));
    VK_CHECK(m_device_dispatch.resetFences(1, &GetCurrentFrame().render_fence));

    uint32_t swapchain_image_index;
    VkResult result = m_device_dispatch.acquireNextImageKHR(
        m_swapchain, one_second_ns, GetCurrentFrame().swapchain_semaphore, nullptr, &swapchain_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ResetSwapchain();
        return;
    }
    else if (result == VK_TIMEOUT)
    {
        return; // try again next frame
    }

    VkCommandBuffer cmd = GetCurrentFrame().command_buffer;
    VK_CHECK(m_device_dispatch.resetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(m_device_dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
    // COMMAND BEGIN

    FinishPendingUploads(cmd);

    Utils::TransitionImage(&m_device_dispatch, cmd, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);
    DrawBackground(cmd);
    Utils::TransitionImage(&m_device_dispatch, cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    DrawGeometry(cmd);

    Utils::TransitionImage(&m_device_dispatch, cmd, m_draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // copy draw into swapchain
    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    Utils::CopyImageToImage(&m_device_dispatch, cmd, m_draw_image.image, m_swapchain_images[swapchain_image_index],
                            m_draw_extent, m_swapchain_extent);
    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    DrawImgui(cmd, m_swapchain_image_views[swapchain_image_index]);
    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // COMMAND END
    VK_CHECK(m_device_dispatch.endCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = Utils::CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo wait_info = Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                 GetCurrentFrame().swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info =
        Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().render_semaphore);

    VkSubmitInfo2 submit_info = Utils::SubmitInfo(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(m_device_dispatch.queueSubmit2(m_graphics_queue, 1, &submit_info, GetCurrentFrame().render_fence));

    VkPresentInfoKHR present_info =
        Utils::PresentInfo(&m_swapchain, &GetCurrentFrame().render_semaphore, &swapchain_image_index);
    result = m_device_dispatch.queuePresentKHR(m_graphics_queue, &present_info);
    if (result == VK_SUBOPTIMAL_KHR)
    {
        ResetSwapchain();
        return;
    }
    else if (result != VK_SUCCESS)
    {
        abort(); // if this happens, investigate why the heck
    }

    ++frame_number;
}

void VulkanEngine::FinishPendingUploads(VkCommandBuffer cmd)
{
    for (PendingMeshUpload& mesh : m_pending_uploads)
    {
        VkBufferCopy vertex_copy{};
        vertex_copy.dstOffset = 0;
        vertex_copy.srcOffset = 0;
        vertex_copy.size = mesh.vertex_buffer_size;

        VkBufferCopy index_copy{};
        index_copy.dstOffset = 0;
        index_copy.srcOffset = mesh.vertex_buffer_size;
        index_copy.size = mesh.index_buffer_size;

        m_device_dispatch.cmdCopyBuffer(cmd, mesh.staging_buffer.buffer, mesh.target_mesh.vertex_buffer.buffer, 1,
                                        &vertex_copy);
        m_device_dispatch.cmdCopyBuffer(cmd, mesh.staging_buffer.buffer, mesh.target_mesh.index_buffer.buffer, 1,
                                        &index_copy);

        // #TODO: we need to destroy this after this frame is done with it, not right now.
        // DestroyBuffer(mesh.staging_buffer);
    }

    m_pending_uploads.clear();
}

void VulkanEngine::DrawBackground(VkCommandBuffer cmd)
{
    ComputeEffect& effect = m_compute_effects[m_current_effect];
    m_device_dispatch.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
    m_device_dispatch.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1,
                                            &m_draw_image_descriptors, 0, nullptr);

    m_device_dispatch.cmdPushConstants(cmd, effect.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                       sizeof(effect.push_constants), &effect.push_constants);

    m_device_dispatch.cmdDispatch(cmd, uint32_t(std::ceil(float(m_draw_extent.width) / 16.0f)),
                                  uint32_t(std::ceil(float(m_draw_extent.height) / 16.0f)), 1);
}

void VulkanEngine::DrawGeometry(VkCommandBuffer cmd)
{
    VkRenderingAttachmentInfo color_attachment = Utils::AttachmentInfo(m_draw_image.image_view, nullptr);

    VkClearValue clear_value{};
    clear_value.depthStencil.depth = 0.0f; // zero is far in reversed depth

    VkRenderingAttachmentInfo depth_attachment =
        Utils::AttachmentInfo(m_depth_image.image_view, &clear_value, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkRenderingInfo render_info = Utils::RenderingInfo(&color_attachment, &depth_attachment, m_draw_extent);

    m_device_dispatch.cmdBeginRendering(cmd, &render_info);

    m_device_dispatch.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_mesh_pipeline);
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = float(m_draw_extent.width);
    viewport.height = float(m_draw_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    m_device_dispatch.cmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = VkOffset2D{0, 0};
    scissor.extent = m_draw_extent;

    m_device_dispatch.cmdSetScissor(cmd, 0, 1, &scissor);

    GPUDrawPushConstants push_constants;
    push_constants.vertex_buffer_address = m_default_mesh->buffers.vertex_buffer_address;

    glm::mat4 view = glm::translate(glm::vec3{0, 0, -1.0f});
    // camera projection
    glm::mat4 projection =
        glm::perspective(glm::radians(70.f), (float)m_draw_extent.width / (float)m_draw_extent.height, 10000.f, 0.1f);

    // invert the Y direction on projection matrix so that we are more similar
    // to opengl and gltf axis
    projection[1][1] *= -1;

    // rotate over time
    float angle = float(frame_number % 360);
    view = glm::rotate(view, glm::radians(angle), glm::vec3(0, 1, 0));

    push_constants.world_matrix = projection * view;

    m_device_dispatch.cmdPushConstants(cmd, m_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                       sizeof(push_constants), &push_constants);
    m_device_dispatch.cmdBindIndexBuffer(cmd, m_default_mesh->buffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    m_device_dispatch.cmdDrawIndexed(cmd, m_default_mesh->surfaces[0].index_count, 1,
                                     m_default_mesh->surfaces[0].first_index, 0, 0);

    m_device_dispatch.cmdEndRendering(cmd);
}

void VulkanEngine::DrawImgui(VkCommandBuffer cmd, VkImageView target_image_view)
{
    VkRenderingAttachmentInfo attachment_info = Utils::AttachmentInfo(target_image_view, nullptr);
    // swapchain extent because we're drawing directly onto it
    constexpr VkRenderingAttachmentInfo* depth_attachment_info = nullptr;
    VkRenderingInfo rendering_info = Utils::RenderingInfo(&attachment_info, depth_attachment_info, m_swapchain_extent);

    m_device_dispatch.cmdBeginRendering(cmd, &rendering_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    m_device_dispatch.cmdEndRendering(cmd);
}

void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VkCommandBuffer cmd = m_immediate_command_buffer;

    VK_CHECK(m_device_dispatch.resetFences(1, &m_immediate_fence));
    VK_CHECK(m_device_dispatch.resetCommandBuffer(m_immediate_command_buffer, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(m_device_dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
    // COMMAND BEGIN

    function(cmd);

    // COMMAND END
    VK_CHECK(m_device_dispatch.endCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = Utils::CommandBufferSubmitInfo(cmd);

    VkSubmitInfo2 submit_info = Utils::SubmitInfo(&cmd_info, nullptr, nullptr);

    VK_CHECK(m_device_dispatch.queueSubmit2(m_graphics_queue, 1, &submit_info, m_immediate_fence));

    VK_CHECK(m_device_dispatch.waitForFences(1, &m_immediate_fence, true, 1'000'000'000));
}

void VulkanEngine::Update(double delta_ms)
{
    if (stop_rendering)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    ImGui::Render();

    Draw(delta_ms);
}

AllocatedBuffer VulkanEngine::CreateBuffer(size_t allocation_size, VkBufferUsageFlags usage,
                                           VmaMemoryUsage memory_usage, const char* debug_name)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.size = allocation_size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.usage = memory_usage;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer buffer;

    VK_CHECK(vmaCreateBuffer(m_allocator, &buffer_info, &alloc_create_info, &buffer.buffer, &buffer.allocation,
                             &buffer.allocation_info));
    SetAllocationName(buffer.allocation, debug_name);

    return buffer;
}

void VulkanEngine::DestroyBuffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
}

GPUMeshBuffers VulkanEngine::UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers buffers{};

    // storage and shader device address to make VB an SSBO that we can access through vertex pulling. Transfer so we
    // can copy into them.
    VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    buffers.vertex_buffer =
        CreateBuffer(vertex_buffer_size, vertex_usage, VMA_MEMORY_USAGE_GPU_ONLY, "buffer_mesh_vertex");

    VkBufferDeviceAddressInfo device_address{};
    device_address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    device_address.pNext = nullptr;
    device_address.buffer = buffers.vertex_buffer.buffer;
    buffers.vertex_buffer_address = m_device_dispatch.getBufferDeviceAddress(&device_address);

    VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffers.index_buffer = CreateBuffer(index_buffer_size, index_usage, VMA_MEMORY_USAGE_GPU_ONLY, "buffer_mesh_index");

    // the buffers are created. Now we need to do the same thing basically and create a staging buffer.

    AllocatedBuffer staging = CreateBuffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY, "buffer_mesh_staging");

    void* data = staging.allocation->GetMappedData();
    memcpy(data, vertices.data(), vertex_buffer_size);
    memcpy(static_cast<char*>(data) + vertex_buffer_size, indices.data(), index_buffer_size);

    if (m_immediate_uploads_enabled)
    {
        ImmediateSubmit([this, staging, vertex_buffer_size, index_buffer_size, buffers](VkCommandBuffer cmd) {
            VkBufferCopy vertex_copy{};
            vertex_copy.dstOffset = 0;
            vertex_copy.srcOffset = 0;
            vertex_copy.size = vertex_buffer_size;

            VkBufferCopy index_copy{};
            index_copy.dstOffset = 0;
            index_copy.srcOffset = vertex_buffer_size;
            index_copy.size = index_buffer_size;

            m_device_dispatch.cmdCopyBuffer(cmd, staging.buffer, buffers.vertex_buffer.buffer, 1, &vertex_copy);
            m_device_dispatch.cmdCopyBuffer(cmd, staging.buffer, buffers.index_buffer.buffer, 1, &index_copy);
        });

        // destroy the staging buffer. We have no use for it anymore
        DestroyBuffer(staging);
    }
    else
    {
        m_pending_uploads.push_back({vertex_buffer_size, index_buffer_size, buffers, staging});
    }

    return buffers;
}

bool VulkanEngine::InitVulkan()
{
    vkb::InstanceBuilder builder;

    builder.set_app_name("Vulkan Engine")
        .request_validation_layers(m_use_validation_layers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0);

    vkb::Result<vkb::Instance> build_result = builder.build();
    if (build_result.has_value() == false)
    {
        return false;
    }

    vkb::Instance vkb_instance = build_result.value();
    m_instance_dispatch = vkb::InstanceDispatchTable(vkb_instance.instance, vkb_instance.fp_vkGetInstanceProcAddr);
    m_get_instance_proc_addr = vkb_instance.fp_vkGetInstanceProcAddr;
    m_get_device_proc_addr = vkb_instance.fp_vkGetDeviceProcAddr;

    m_instance = vkb_instance.instance;
    m_debug_messenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector(vkb_instance);
    vkb::PhysicalDevice vkb_gpu = selector.set_minimum_version(1, 3)
                                      .set_required_features_13(features13)
                                      .set_required_features_12(features12)
                                      .set_surface(m_surface)
                                      .select()
                                      .value();

    vkb::DeviceBuilder deviceBuilder(vkb_gpu);
    vkb::Device vkb_device = deviceBuilder.build().value();

    m_gpu = vkb_gpu.physical_device;
    m_device = vkb_device.device;
    m_device_dispatch = vkb::DispatchTable(m_device, vkb_device.fp_vkGetDeviceProcAddr);

    m_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // make sure we destroy the surface when we're done
    m_deletion_queue.PushFunction("main surface",
                                  [this]() { m_instance_dispatch.destroySurfaceKHR(m_surface, nullptr); });

    return true;
}

void VulkanEngine::InitAllocator()
{
    VmaVulkanFunctions vulkan_functions{};
    vulkan_functions.vkGetInstanceProcAddr = m_get_instance_proc_addr;
    vulkan_functions.vkGetDeviceProcAddr = m_get_device_proc_addr;

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.instance = m_instance;
    allocator_info.device = m_device;
    allocator_info.physicalDevice = m_gpu;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocator_info.pVulkanFunctions = &vulkan_functions;
    vmaCreateAllocator(&allocator_info, &m_allocator);

    m_deletion_queue.PushFunction("vmaAllocator", [this]() { vmaDestroyAllocator(m_allocator); });
}

void VulkanEngine::CreateSwapchain(uint32_t width, uint32_t height)
{
    // destroy in case it already exists
    for (std::size_t i = 0; i < m_swapchain_image_views.size(); ++i)
    {
        m_device_dispatch.destroyImageView(m_swapchain_image_views[i], nullptr);
    }
    m_device_dispatch.destroySwapchainKHR(m_swapchain, nullptr);

    vkb::SwapchainBuilder builder(m_gpu, m_device, m_surface);

    m_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain =
        builder
            .set_desired_format(
                VkSurfaceFormatKHR{.format = m_swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // #TODO: Implement MAILBOX present
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    m_swapchain_extent = vkb_swapchain.extent;
    m_swapchain = vkb_swapchain.swapchain;
    m_swapchain_images = vkb_swapchain.get_images().value();
    m_swapchain_image_views = vkb_swapchain.get_image_views().value();

    for (std::size_t i = 0; i < m_swapchain_image_views.size(); ++i)
    {
        m_deletion_queue.PushFunction("swapchain image view", [i, this]() {
            m_device_dispatch.destroyImageView(m_swapchain_image_views[i], nullptr);
        });
    }
    m_deletion_queue.PushFunction("swapchain",
                                  [this]() { m_device_dispatch.destroySwapchainKHR(m_swapchain, nullptr); });
}

void VulkanEngine::CreateDrawImage()
{
    VkExtent3D image_extent{uint32_t(float(m_window_extent.width) * m_backbuffer_scale),
                            uint32_t(float(m_window_extent.height) * m_backbuffer_scale), 1};

    // draw image defines the resolution we render at. Can be different to swapchain resolution
    m_draw_extent = VkExtent2D{image_extent.width, image_extent.height};

    VkFormat image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkImageUsageFlags usage_flags{};
    usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImageAspectFlagBits aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    VkMemoryPropertyFlags additional_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    m_draw_image = AllocateImage(image_extent.width, image_extent.height, image_format, usage_flags, memory_usage,
                                 aspect_flags, additional_flags, "image_draw");

    m_deletion_queue.PushFunction("draw image", [this]() {
        m_device_dispatch.destroyImageView(m_draw_image.image_view, nullptr);
        vmaDestroyImage(m_allocator, m_draw_image.image, m_draw_image.allocation);
    });
}

void VulkanEngine::CreateDepthImage()
{
    VkFormat image_format = VK_FORMAT_D32_SFLOAT;
    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImageAspectFlagBits aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    VkMemoryPropertyFlags additional_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_depth_image = AllocateImage(m_draw_extent.width, m_draw_extent.height, image_format, usage_flags, memory_usage,
                                  aspect_flags, additional_flags, "image_depth");

    m_deletion_queue.PushFunction("depth image", [this]() {
        m_device_dispatch.destroyImageView(m_depth_image.image_view, nullptr);
        vmaDestroyImage(m_allocator, m_depth_image.image, m_depth_image.allocation);
    });
}

AllocatedImage VulkanEngine::AllocateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                                           VmaMemoryUsage memory_usage, VkImageAspectFlagBits aspect_flags,
                                           VkMemoryPropertyFlags additional_flags, const char* debug_name)
{
    AllocatedImage image{};

    VkExtent3D image_extent{width, height, 1};
    image.image_extent = image_extent;
    image.image_format = format;

    VkImageCreateInfo image_info = Utils::ImageCreateInfo(format, usage, image_extent);

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = memory_usage;
    allocation_info.requiredFlags = additional_flags;

    vmaCreateImage(m_allocator, &image_info, &allocation_info, &image.image, &image.allocation, nullptr);
    SetAllocationName(image.allocation, debug_name);

    VkImageViewCreateInfo image_view_info = Utils::ImageViewCreateInfo(format, image.image, aspect_flags);
    VK_CHECK(m_device_dispatch.createImageView(&image_view_info, nullptr, &image.image_view));

    return image;
}

void VulkanEngine::ResetSwapchain()
{
    CreateSwapchain(m_window_extent.width, m_window_extent.height);
    CreateDrawImage();
    CreateDepthImage();
}

void VulkanEngine::InitCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo =
        Utils::CommandPoolCreateInfo(m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (std::size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(m_device_dispatch.createCommandPool(&commandPoolInfo, nullptr, &m_frames[i].command_pool));

        VkCommandBufferAllocateInfo cmdAllocInfo = Utils::CommandBufferAllocateInfo(m_frames[i].command_pool, 1);

        VK_CHECK(m_device_dispatch.allocateCommandBuffers(&cmdAllocInfo, &m_frames[i].command_buffer));

        m_deletion_queue.PushFunction(
            "command pool", [i, this]() { m_device_dispatch.destroyCommandPool(m_frames[i].command_pool, nullptr); });
    }

    if (m_immediate_uploads_enabled == false)
    {
        return;
    }

    // immediate command buffer for short tasks
    VK_CHECK(m_device_dispatch.createCommandPool(&commandPoolInfo, nullptr, &m_immediate_command_pool));
    VkCommandBufferAllocateInfo cmdAllocInfo = Utils::CommandBufferAllocateInfo(m_immediate_command_pool, 1);
    VK_CHECK(m_device_dispatch.allocateCommandBuffers(&cmdAllocInfo, &m_immediate_command_buffer));
    m_deletion_queue.PushFunction("Immediate command pool", [this]() {
        m_device_dispatch.destroyCommandPool(m_immediate_command_pool, nullptr);
    });
}

void VulkanEngine::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo(0);

    for (std::size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(m_device_dispatch.createFence(&fenceCreateInfo, nullptr, &m_frames[i].render_fence));

        m_deletion_queue.PushFunction(
            "fence", [i, this]() { m_device_dispatch.destroyFence(m_frames[i].render_fence, nullptr); });

        VK_CHECK(m_device_dispatch.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frames[i].swapchain_semaphore));
        VK_CHECK(m_device_dispatch.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frames[i].render_semaphore));

        m_deletion_queue.PushFunction("semaphores x2", [i, this]() {
            m_device_dispatch.destroySemaphore(m_frames[i].render_semaphore, nullptr);
            m_device_dispatch.destroySemaphore(m_frames[i].swapchain_semaphore, nullptr);
        });
    }

    if (m_immediate_uploads_enabled == false)
    {
        return;
    }

    // immediate command buffer for short tasks
    VK_CHECK(m_device_dispatch.createFence(&fenceCreateInfo, nullptr, &m_immediate_fence));
    m_deletion_queue.PushFunction("Immediate fence",
                                  [this]() { m_device_dispatch.destroyFence(m_immediate_fence, nullptr); });
}

void VulkanEngine::InitDescriptors()
{
    // 10 sets with 1 image each
    std::vector<Utils::DescriptorAllocator::PoolSizeRatio> sizes{{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
    m_descriptor_allocator.InitPool(m_device_dispatch, 10, sizes);

    // descriptor set layout for the compute draw
    {
        Utils::DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_draw_image_descriptor_layout = builder.Build(m_device_dispatch, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_draw_image_descriptors = m_descriptor_allocator.Allocate(m_device_dispatch, m_draw_image_descriptor_layout);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = m_draw_image.image_view;

    VkWriteDescriptorSet write_set{};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.descriptorCount = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_set.pImageInfo = &image_info;
    write_set.dstBinding = 0;
    write_set.dstSet = m_draw_image_descriptors;

    m_device_dispatch.updateDescriptorSets(1, &write_set, 0, nullptr);

    m_deletion_queue.PushFunction("descriptors", [this]() {
        m_descriptor_allocator.DestroyPool(m_device_dispatch);
        m_device_dispatch.destroyDescriptorSetLayout(m_draw_image_descriptor_layout, nullptr);
    });
}

bool VulkanEngine::InitPipelines()
{
    if (InitBackgroundPipelines() == false)
    {
        return false;
    }

    return InitMeshPipeline();
}

bool VulkanEngine::InitBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;
    layout_info.pSetLayouts = &m_draw_image_descriptor_layout;
    layout_info.setLayoutCount = 1;

    VkPushConstantRange push_constants_info{};
    push_constants_info.offset = 0;
    push_constants_info.size = sizeof(PushConstants);
    push_constants_info.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constants_info;

    VK_CHECK(m_device_dispatch.createPipelineLayout(&layout_info, nullptr, &m_gradient_pipeline_layout));

    m_deletion_queue.PushFunction(
        "pipeline layout", [this]() { m_device_dispatch.destroyPipelineLayout(m_gradient_pipeline_layout, nullptr); });

    // create background effects
    ComputeEffect sky_effect;
    if (CreateComputeEffect("sky", "../data/shader/sky.comp.spv", m_device_dispatch, m_gradient_pipeline_layout,
                            m_deletion_queue, &sky_effect) == false)
    {
        return false;
    }
    sky_effect.push_constants.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);
    m_compute_effects.emplace_back(sky_effect);

    ComputeEffect gradient;
    if (CreateComputeEffect("gradient_color", "../data/shader/gradient_color.comp.spv", m_device_dispatch,
                            m_gradient_pipeline_layout, m_deletion_queue, &gradient) == false)
    {
        return false;
    }
    gradient.push_constants.data1 = glm::vec4(1, 0, 0, 1);
    gradient.push_constants.data2 = glm::vec4(0, 0, 1, 1);
    m_compute_effects.emplace_back(gradient);

    return true;
}

bool VulkanEngine::InitMeshPipeline()
{
    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(GPUDrawPushConstants);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;
    layout_info.flags = 0;
    layout_info.pSetLayouts = nullptr;
    layout_info.setLayoutCount = 0;
    layout_info.pPushConstantRanges = &push_constant_range;
    layout_info.pushConstantRangeCount = 1;

    if (m_device_dispatch.createPipelineLayout(&layout_info, nullptr, &m_mesh_pipeline_layout) != VK_SUCCESS)
    {
        std::cout << "[!] Failed to create Mesh pipeline layout." << std::endl;
        return false;
    }

    m_deletion_queue.PushFunction(
        "mesh pipeline layout", [this]() { m_device_dispatch.destroyPipelineLayout(m_mesh_pipeline_layout, nullptr); });

    VkShaderModule vertex_shader{};
    if (Utils::LoadShaderModule(m_device_dispatch, "../data/shader/mesh.vert.spv", &vertex_shader) == false)
    {
        return false;
    }
    VkShaderModule frag_shader{};
    if (Utils::LoadShaderModule(m_device_dispatch, "../data/shader/triangle.frag.spv", &frag_shader) == false)
    {
        return false;
    }

    m_mesh_pipeline = Utils::PipelineBuilder()
                          .SetName("mesh")
                          .SetLayout(m_mesh_pipeline_layout)
                          .AddFragmentShader(frag_shader)
                          .AddVertexShader(vertex_shader)
                          .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                          .SetPolygonMode(VK_POLYGON_MODE_FILL)
                          .SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE)
                          .SetColorAttachmentFormat(m_draw_image.image_format)
                          .SetDepthFormat(VK_FORMAT_D32_SFLOAT)
                          .SetMultisamplingNone()
                          .EnableBlendingAlpha()
                          .EnableDepthTest(VK_COMPARE_OP_GREATER_OR_EQUAL)
                          .BuildPipeline(m_device_dispatch);
    if (m_mesh_pipeline == VK_NULL_HANDLE)
    {
        return false;
    }

    m_device_dispatch.destroyShaderModule(vertex_shader, nullptr);
    m_device_dispatch.destroyShaderModule(frag_shader, nullptr);

    m_deletion_queue.PushFunction("mesh pipeline", [this]() {
        m_device_dispatch.destroyPipelineLayout(m_mesh_pipeline_layout, nullptr);
        m_device_dispatch.destroyPipeline(m_mesh_pipeline, nullptr);
    });

    return true;
}

void VulkanEngine::InitDefaultData()
{
    auto loaded_meshes = Utils::LoadGltfMeshes(this, std::filesystem::path{"../data/resources/BarramundiFish.glb"});
    if (loaded_meshes.has_value() == false || loaded_meshes->empty())
    {
        std::cout << "[!] Failed to load default mesh. Will probably crash." << std::endl;
    }
    else
    {
        m_default_mesh = loaded_meshes->at(0);
    }

    m_deletion_queue.PushFunction("default mesh", [this]() {
        DestroyBuffer(m_default_mesh->buffers.vertex_buffer);
        DestroyBuffer(m_default_mesh->buffers.index_buffer);
    });
}

void VulkanEngine::InitImgui()
{
    // Mostly copied from the examples
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imgui_descriptor_pool;
    VK_CHECK(m_device_dispatch.createDescriptorPool(&pool_info, nullptr, &imgui_descriptor_pool));

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* function_name, void* engine) {
            VulkanEngine* engine_casted = reinterpret_cast<VulkanEngine*>(engine);
            return engine_casted->m_get_instance_proc_addr(engine_casted->m_instance, function_name);
        },
        this);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(m_window);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_gpu;
    init_info.Device = m_device;
    init_info.Queue = m_graphics_queue;
    init_info.DescriptorPool = imgui_descriptor_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    init_info.PipelineRenderingCreateInfo = {};
    init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain_format;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    m_deletion_queue.PushFunction("imgui", [this, imgui_descriptor_pool]() {
        ImGui_ImplVulkan_Shutdown();
        m_device_dispatch.destroyDescriptorPool(imgui_descriptor_pool, nullptr);
    });
}

void VulkanEngine::SetAllocationName([[maybe_unused]] VmaAllocation allocation, [[maybe_unused]] const char* name)
{
#ifdef CHEEKY_ENABLE_MEMORY_TRACKING
    vmaSetAllocationName(m_allocator, allocation, name);
#endif
}