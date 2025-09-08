#include "Renderer/VkEngine.h"

#include "Renderer/Material.h"
#include "Renderer/MaterialInterface.h"
#include "Renderer/RenderObject.h"
#include "Renderer/Renderable.h"
#include "Renderer/Utility/DebugPanels.h"
#include "Renderer/Utility/UploadRequest.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/Utility/VkImages.h"
#include "Renderer/Utility/VkInitialisers.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/Utility/VkPipelines.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/ImGUI.h"
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>
#include <array>
#include <glm/ext/vector_float4.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#define VK_DEVICE_CALL(device, function, ...)                                                                \
    reinterpret_cast<PFN_##function>(m_get_device_proc_addr(device, #function))(device, __VA_ARGS__);

#define VK_INSTANCE_CALL(instance, function, ...)                                                            \
    reinterpret_cast<PFN_##function>(m_get_instance_proc_addr(instance, #function))(instance, __VA_ARGS__);

namespace Renderer
{
    VulkanEngine::VulkanEngine(
        uint32_t window_width,
        uint32_t window_height,
        SDL_Window* window,
        float backbuffer_scale,
        bool use_validation_layers,
        bool immediate_uploads
    ) :
        m_backbuffer_scale(backbuffer_scale),
        m_window_extent({ window_width, window_height }),
        m_window(window),
        m_use_validation_layers(use_validation_layers),
        m_force_all_uploads_immediate(immediate_uploads)
    {
    }

    bool VulkanEngine::Init()
    {
        if (InitVulkan() == false)
        {
            return false;
        }

        InitAllocator();
        CreateSwapchain(m_window_extent.width, m_window_extent.height);
        InitCommands();
        InitSyncStructures();
        InitFrameDescriptors();
        InitDefaultDescriptors();

        // InitPipelines is where we initialise materials for the first time so the material interface needs
        // to be ready by then.
        m_material_interface = MaterialEngineInterface{ &m_device_dispatch,
                                                        &m_allocator,
                                                        VKENGINE_DRAW_IMAGE_FORMAT,
                                                        VKENGINE_DEPTH_IMAGE_FORMAT,
                                                        m_scene_data_descriptor_layout,
                                                        this };

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

        // nuke the frame descriptors
        for (FrameData& frame : m_frames)
        {
            frame.deletion_queue.Flush();
        }

        // swapchain isn't handled by the deletion queue because it gets recreated at runtime
        DestroySwapchain();

        // destroy all resource storages
        m_image_storage.Clear(*this);
        m_buffer_storage.Clear(*this);
        m_mesh_storage.Clear(*this);

        render_scenes.clear();
        main_scene = 0;

        m_deletion_queue.Flush();

        VK_DEVICE_CALL(m_device, vkDestroyDevice, nullptr);
        vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
        VK_INSTANCE_CALL(m_instance, vkDestroyInstance, nullptr);
        is_initialised = false;
    }

    void VulkanEngine::Update()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Graphics"))
            {
                ImGui::Checkbox("Engine Settings", &m_draw_engine_settings);
                ImGui::Checkbox("Scene Editor", &m_draw_scene_editor);
                ImGui::Checkbox("Resource Debugger", &m_draw_resource_debugger);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (m_draw_resource_debugger)
        {
            if (ImGui::Begin("Resource Debugger", &m_draw_resource_debugger))
            {
                if (ImGui::CollapsingHeader("Images"))
                {
                    ImGui::PushID("Images");
                    Renderer::Debug::DrawStorageTableImGui(*this, m_image_storage);
                    ImGui::PopID();
                }

                if (ImGui::CollapsingHeader("Buffers"))
                {
                    ImGui::PushID("Buffers");
                    Renderer::Debug::DrawStorageTableImGui(*this, m_buffer_storage);
                    ImGui::PopID();
                }

                if (ImGui::CollapsingHeader("Meshes"))
                {
                    ImGui::PushID("Meshes");
                    Renderer::Debug::DrawStorageTableImGui(*this, m_mesh_storage);
                    ImGui::PopID();
                }
            }
            ImGui::End();
        }

        if (m_draw_engine_settings)
        {
            if (ImGui::Begin("Engine Settings", &m_draw_engine_settings))
            {
                ImGui::Text("Frame: %d", frame_number);
                ImGui::Text("Backbuffer Scale: %.2f", m_backbuffer_scale);
                ImGui::Text(
                    "Swapchain Resolution: %dx%d", m_swapchain_extent.width, m_swapchain_extent.height
                );
                ImGui::Text("Window Resolution: %dx%d", m_window_extent.width, m_window_extent.height);
            }

            if (ImGui::CollapsingHeader("Scene Lighting"))
            {
                ImGui::ColorEdit3(
                    "Ambient Colour", &render_scenes[main_scene].frame_context.ambient_colour.r
                );
                ImGui::ColorEdit3("Light Colour", &render_scenes[main_scene].frame_context.light_colour.r);
                ImGui::SliderFloat3(
                    "Light Direction", &render_scenes[main_scene].frame_context.light_direction.x, -1.0f, 1.0f
                );
            }

            ImGui::End();
        }

        if (m_draw_scene_editor)
        {
            if (ImGui::Begin("Scenes", &m_draw_scene_editor))
            {
                if (ImGui::BeginTabBar("scene_tabs"))
                {
                    for (Scene& scene : render_scenes)
                    {
                        if (ImGui::BeginTabItem(scene.scene_name.data()))
                        {
                            ImGui::PushID(&scene);
                            Renderer::Debug::DrawSceneContentsImGui(*this, scene);
                            ImGui::PopID();
                            ImGui::EndTabItem();
                        }
                    }

                    ImGui::EndTabBar();
                }
            }

            ImGui::End();
        }

        ImGui::Render();

        if (stop_rendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return;
        }

        if (m_resize_requested)
        {
            ResizeSwapchain();
            return; // no render while resizing (or minimised!)
        }

        Draw();
    }

#pragma region Allocation_Destruction

    BufferHandle VulkanEngine::CreateBuffer(
        size_t allocation_size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage,
        VmaAllocationCreateFlags allocation_flags,
        const char* debug_name
    )
    {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.pNext = nullptr;
        buffer_info.size = allocation_size;
        buffer_info.usage = usage;

        VmaAllocationCreateInfo alloc_create_info{};
        alloc_create_info.usage = memory_usage;
        alloc_create_info.flags = allocation_flags;
        AllocatedBuffer buffer;

        VK_CHECK(vmaCreateBuffer(
            m_allocator,
            &buffer_info,
            &alloc_create_info,
            &buffer.buffer,
            &buffer.allocation,
            &buffer.allocation_info
        ));
        SetAllocationName(buffer.allocation, debug_name);

        return m_buffer_storage.AddResource(buffer, debug_name);
    }

    BufferHandle VulkanEngine::CreateBuffer(
        void* buffer_data, size_t buffer_size, VkBufferUsageFlags usage, const char* debug_name
    )
    {
        // we set up the flags so that the memory can end up in either BAR or VRAM that is
        // inaccessible by host. If it ends up in bar, we can simply map it and copy into it. If
        // not, we need to create a staging buffer and upload it with a command.
        VkBufferUsageFlags created_buffer_usage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaMemoryUsage allocation_usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocation_flags =
            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        BufferHandle buffer =
            CreateBuffer(buffer_size, created_buffer_usage, allocation_usage, allocation_flags, debug_name);

        VkMemoryPropertyFlags memory_properties;
        vmaGetAllocationMemoryProperties(m_allocator, buffer->allocation, &memory_properties);

        if (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            // this is mappable! We simply map it and immediately copy the data over.
            vmaCopyMemoryToAllocation(m_allocator, buffer_data, buffer->allocation, 0, buffer_size);
        }
        else
        {
            VkBufferUsageFlags staging_buffer_usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VmaMemoryUsage staging_memory_usage = VMA_MEMORY_USAGE_AUTO;
            VmaAllocationCreateFlags allocation_flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            BufferHandle staging_buffer = CreateBuffer(
                buffer_size, staging_buffer_usage, staging_memory_usage, allocation_flags, debug_name
            );

            vmaCopyMemoryToAllocation(m_allocator, buffer_data, staging_buffer->allocation, 0, buffer_size);

            // no offsets, just an honest to god copy
            std::unique_ptr<Utils::IUploadRequest> upload_request =
                std::make_unique<Utils::BufferUploadRequest>(
                    buffer_size, staging_buffer, buffer, Utils::UploadType::Deferred, 0, 0, debug_name
                );
            RequestUpload(std::move(upload_request));
        }

        return buffer;
    }

    void VulkanEngine::DestroyBuffer(const AllocatedBuffer& buffer)
    {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    }

    ImageHandle VulkanEngine::AllocateImage(
        VkExtent3D image_extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VmaMemoryUsage memory_usage,
        VkImageAspectFlagBits aspect_flags,
        VkMemoryPropertyFlags required_memory_flags,
        VmaAllocationCreateFlags allocation_flags,
        bool mipmapped,
        const char* debug_name
    )
    {
        AllocatedImage image{};
        image.image_extent = image_extent;
        image.image_format = format;

        // if image debugging is enabled, any image might be sampled by imgui.
        if (m_enable_image_debugging)
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        VkImageCreateInfo image_info = Utils::ImageCreateInfo(format, usage, image_extent);
        if (mipmapped)
        {
            // log2(max_dimension) - 3 gives a reasonable mip count
            double mipLevels = std::log2(std::max(image_extent.height, image_extent.width)) - 3.0f;
            // more than 10 is useless though
            image_info.mipLevels = std::min(static_cast<uint32_t>(mipLevels), 10u);
        }

        VmaAllocationCreateInfo allocation_info{};
        allocation_info.usage = memory_usage;
        allocation_info.requiredFlags = required_memory_flags;
        allocation_info.flags = allocation_flags;

        VkResult result = vmaCreateImage(
            m_allocator, &image_info, &allocation_info, &image.image, &image.allocation, nullptr
        );
        VK_CHECK(result);
        SetAllocationName(image.allocation, debug_name);

        VkImageViewCreateInfo image_view_info = Utils::ImageViewCreateInfo(format, image.image, aspect_flags);
        image_view_info.subresourceRange.levelCount = image_info.mipLevels;
        VK_CHECK(m_device_dispatch.createImageView(&image_view_info, nullptr, &image.image_view));

        ImageHandle handle = m_image_storage.AddResource(image, debug_name);

        m_debug_image_map[handle->image] = ImGui_ImplVulkan_AddTexture(
            m_default_sampler_nearest, handle->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        return handle;
    }

    ImageHandle VulkanEngine::AllocateImage(
        void* image_data,
        VkExtent3D image_extent,
        VkFormat format,
        VkImageUsageFlags image_usage,
        VkImageLayout layout,
        bool mipmapped,
        const char* debug_name
    )
    {
        // we'll try to use the BAR, which is addressable by both CPU and GPU. If cannot use, we'll
        // just do a staging buffer and copy from that.
        const size_t image_data_size = size_t(image_extent.width) * size_t(image_extent.height) *
                                       size_t(image_extent.depth) *
                                       4ul; // assuming RGBA8, 1 byte for each component

        // because we might be allocating into non host visible memory, we need to make sure the target image
        // can be copied into
        VkImageUsageFlags target_image_usage = image_usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO;
        VkMemoryPropertyFlags required_memory_flags =
            0; // actually anything is fine, we want the most performant one
        VmaAllocationCreateFlags allocation_flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT; // this makes is so that
        // we might get a
        // non-mappable memory

        VkImageAspectFlagBits aspect_flags =
            VK_IMAGE_ASPECT_COLOR_BIT; // we assume RGBA8 so has to be colour.

        ImageHandle image = AllocateImage(
            image_extent,
            format,
            target_image_usage,
            memory_usage,
            aspect_flags,
            required_memory_flags,
            allocation_flags,
            mipmapped,
            debug_name
        );

        VkMemoryPropertyFlags memory_properties;
        vmaGetAllocationMemoryProperties(m_allocator, image->allocation, &memory_properties);

        // since the wise allocator decided that the most optimal place for the image to be read from is
        // not host visible, we need to create a staging image that is visible on host and copy that over
        // with a command buffer.
        BufferHandle staging_buffer =
            CreateBuffer(image_data, image_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, debug_name);

        std::unique_ptr<Utils::IUploadRequest> upload_request = std::make_unique<Utils::ImageUploadRequest>(
            image_extent, staging_buffer, image, Utils::UploadType::Deferred, layout, debug_name
        );
        RequestUpload(std::move(upload_request));

        return image;
    }

    void VulkanEngine::DestroyImage(const AllocatedImage& image)
    {
        // nuke the debug image
        if (m_debug_image_map.contains(image.image))
        {
            ImGui_ImplVulkan_RemoveTexture(m_debug_image_map[image.image]);
            m_debug_image_map.erase(image.image);
        }

        m_device_dispatch.destroyImageView(image.image_view, nullptr);
        vmaDestroyImage(m_allocator, image.image, image.allocation);
    }

    GPUMeshBuffers VulkanEngine::UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
    {
        const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
        const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

        GPUMeshBuffers buffers{};

        // storage and shader device address to make VB an SSBO that we can access through vertex
        // pulling. Transfer so we can copy into them.
        VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        buffers.vertex_buffer = CreateBuffer(
            vertex_buffer_size, vertex_usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "buffer_mesh_vertex"
        );

        VkBufferDeviceAddressInfo device_address{};
        device_address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        device_address.pNext = nullptr;
        device_address.buffer = buffers.vertex_buffer->buffer;
        buffers.vertex_buffer_address = m_device_dispatch.getBufferDeviceAddress(&device_address);

        VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffers.index_buffer = CreateBuffer(
            index_buffer_size, index_usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "buffer_mesh_index"
        );

        // the buffers are created. Now we need to do the same thing basically and create a staging
        // buffer.

        BufferHandle staging = CreateBuffer(
            vertex_buffer_size + index_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            "buffer_mesh_staging"
        );

        vmaCopyMemoryToAllocation(m_allocator, vertices.data(), staging->allocation, 0, vertex_buffer_size);
        vmaCopyMemoryToAllocation(
            m_allocator, indices.data(), staging->allocation, vertex_buffer_size, index_buffer_size
        );

        // we can't do normal buffer upload here because we do two uploads from a single staging buffer.
        // that is not possible with buffer upload requests because those take ownership of the staging buffer
        // so after one is done, and the other gets to the upload, the staging buffer will be destroyed.
        std::unique_ptr<Utils::IUploadRequest> upload_request = std::make_unique<Utils::MeshUploadRequest>(
            vertex_buffer_size, index_buffer_size, buffers, staging, Utils::UploadType::Deferred
        );
        RequestUpload(std::move(upload_request));

        return buffers;
    }

    MeshHandle VulkanEngine::RegisterMeshAsset(MeshAsset&& asset, std::string_view debug_name)
    {
        return m_mesh_storage.AddResource(std::move(asset), debug_name);
    }

    void VulkanEngine::RequestUpload(std::unique_ptr<Utils::IUploadRequest>&& upload_request)
    {
        if (m_force_all_uploads_immediate || upload_request->GetUploadType() == Utils::UploadType::Immediate)
        {
            ImmediateSubmit(
                [this, upload_request = upload_request.get()](VkCommandBuffer cmd)
                {
                    upload_request->ExecuteUpload(*this, cmd);
                }
            );

            upload_request->DestroyResources(*this);
            return;
        }

        std::lock_guard lock(m_pending_upload_mutex);
        m_pending_uploads.push_back(std::move(upload_request));
    }

    void VulkanEngine::DestroyPendingResources()
    {
        m_image_storage.DestroyPendingResources(*this);
        m_buffer_storage.DestroyPendingResources(*this);
        m_mesh_storage.DestroyPendingResources(*this);
    }

    void VulkanEngine::FinishPendingUploads(VkCommandBuffer cmd)
    {
        // some uploads might need to wait until next frame to execute
        std::vector<std::unique_ptr<Utils::IUploadRequest>> next_frame_uploads;

        // consider using a double buffer instead. This might be a big wait.
        std::lock_guard lock(m_pending_upload_mutex);
        for (std::unique_ptr<Utils::IUploadRequest>& request : m_pending_uploads)
        {
            Utils::UploadExecutionResult result = request->ExecuteUpload(*this, cmd);
            if (result == Utils::UploadExecutionResult::RetryNextFrame)
            {
                next_frame_uploads.push_back(std::move(request));
                continue;
            }
            else if (result == Utils::UploadExecutionResult::Failed)
            {
                std::cerr << "[!] Upload request \"" << request->DebugName()
                          << "\" failed to execute. Ignoring." << std::endl;
            }

            m_completed_uploads.emplace_back(std::move(request));
            GetCurrentFrame().deletion_queue.PushFunction(
                "upload request",
                [this, request = m_completed_uploads.back().get()]()
                {
                    request->DestroyResources(*this);

                    // is this a bad way to do this? Search is the easiest but idk
                    std::erase_if(
                        m_completed_uploads,
                        [request](const std::unique_ptr<Utils::IUploadRequest>& ptr)
                        {
                            return ptr.get() == request;
                        }
                    );
                }
            );
        }

        // nothing inside m_pending_uploads is valid anymore
        m_pending_uploads.clear();

        // move the next frame uploads into the pending uploads
        m_pending_uploads = std::move(next_frame_uploads);
    }

    void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
    {
        VkCommandBuffer cmd = m_immediate_command_buffer;

        VK_CHECK(m_device_dispatch.resetFences(1, &m_immediate_fence));
        VK_CHECK(m_device_dispatch.resetCommandBuffer(m_immediate_command_buffer, 0));

        VkCommandBufferBeginInfo cmdBeginInfo =
            Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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

#pragma endregion Allocation_Destruction

#pragma region Draw

    void VulkanEngine::Draw()
    {
        constexpr uint64_t one_second_ns = 1'000'000'000;
        VK_CHECK(m_device_dispatch.waitForFences(1, &GetCurrentFrame().render_fence, true, one_second_ns));
        VK_CHECK(m_device_dispatch.resetFences(1, &GetCurrentFrame().render_fence));

        GetCurrentFrame().deletion_queue.Flush();
        GetCurrentFrame().buffers_in_use.clear();
        GetCurrentFrame().images_in_use.clear();
        GetCurrentFrame().frame_descriptors.ClearDescriptors(m_device_dispatch);

        // this is where we exterminate the resources pending destruction.
        DestroyPendingResources();

        uint32_t swapchain_image_index;
        VkResult result = m_device_dispatch.acquireNextImageKHR(
            m_swapchain, one_second_ns, GetCurrentFrame().swapchain_semaphore, nullptr, &swapchain_image_index
        );
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_resize_requested = true;
            return;
        }
        else if (result == VK_TIMEOUT)
        {
            return; // try again next frame
        }

        VkCommandBuffer cmd = GetCurrentFrame().command_buffer;
        VK_CHECK(m_device_dispatch.resetCommandBuffer(cmd, 0));

        VkCommandBufferBeginInfo cmdBeginInfo =
            Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(m_device_dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
        // COMMAND BEGIN

        FinishPendingUploads(cmd);

        // draw onto draw image.
        for (size_t i = 0; i < render_scenes.size(); ++i)
        {
            Scene& scene = render_scenes[i];

            // might be drawing on a subsection of the image.
            glm::vec2 viewport_extent = scene.viewport_extent;
            if (viewport_extent.x == 0.0f && viewport_extent.y == 0.0f)
            {
                viewport_extent =
                    glm::vec2(scene.draw_image->image_extent.height, scene.draw_image->image_extent.width);
            }

            scene.draw_extent.height = uint32_t(viewport_extent.x * scene.render_scale);
            scene.draw_extent.width = uint32_t(viewport_extent.y * scene.render_scale);

            VkImageLayout current = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout target = VK_IMAGE_LAYOUT_GENERAL;
            Utils::TransitionImage(&m_device_dispatch, cmd, scene.draw_image->image, current, target);
            if (scene.clear_before_draw)
            {
                VkClearColorValue clear_colour;
                clear_colour.float32[0] = 0;
                clear_colour.float32[1] = 0;
                clear_colour.float32[2] = 0;
                clear_colour.float32[3] = 0;
                VkImageSubresourceRange range = Utils::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
                m_device_dispatch.cmdClearColorImage(
                    cmd, scene.draw_image->image, target, &clear_colour, 1, &range
                );
            }
            current = target;
            target = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            Utils::TransitionImage(&m_device_dispatch, cmd, scene.draw_image->image, current, target);
            DrawSceneGeometry(scene, cmd);
            current = target;
            target = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            Utils::TransitionImage(&m_device_dispatch, cmd, scene.draw_image->image, current, target);

            // clear the frame context so it's empty for the next frame
            scene.frame_context = {};
        }

        // copy the main draw into swapchain
        VkImageLayout current = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout target = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        Utils::TransitionImage(
            &m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index], current, target
        );
        Utils::CopyImageToImage(
            &m_device_dispatch,
            cmd,
            render_scenes[main_scene].draw_image->image,
            m_swapchain_images[swapchain_image_index],
            render_scenes[main_scene].draw_extent,
            m_swapchain_extent
        );
        current = target;
        target = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        Utils::TransitionImage(
            &m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index], current, target
        );
        DrawImgui(cmd, m_swapchain_image_views[swapchain_image_index]);
        current = target;
        target = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        Utils::TransitionImage(
            &m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index], current, target
        );

        // if texture debugging, transition all draw images for scenes to shader read only.
        if (m_enable_image_debugging)
        {
            for (Scene& scene : render_scenes)
            {
                Utils::TransitionImage(
                    &m_device_dispatch,
                    cmd,
                    scene.draw_image->image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
            }
        }

        // COMMAND END
        VK_CHECK(m_device_dispatch.endCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmd_info = Utils::CommandBufferSubmitInfo(cmd);

        VkSemaphoreSubmitInfo wait_info = Utils::SemaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame().swapchain_semaphore
        );
        VkSemaphoreSubmitInfo signal_info = Utils::SemaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().render_semaphore
        );

        VkSubmitInfo2 submit_info = Utils::SubmitInfo(&cmd_info, &signal_info, &wait_info);

        VK_CHECK(
            m_device_dispatch.queueSubmit2(m_graphics_queue, 1, &submit_info, GetCurrentFrame().render_fence)
        );

        VkPresentInfoKHR present_info =
            Utils::PresentInfo(&m_swapchain, &GetCurrentFrame().render_semaphore, &swapchain_image_index);
        result = m_device_dispatch.queuePresentKHR(m_graphics_queue, &present_info);
        if (result == VK_SUBOPTIMAL_KHR)
        {
            m_resize_requested = true;
            return;
        }
        else if (result != VK_SUCCESS)
        {
            abort(); // if this happens, investigate why the heck
        }

        ++frame_number;
    }

    void VulkanEngine::DrawSceneGeometry(const Scene& scene, VkCommandBuffer cmd)
    {
        // create the scene data!
        // cpu to gpu so we can skip uploading it. Hopefully the data is small enough to fit in the
        // GPU cache so it won't need to read from system memory. if that is not the case, lol
        BufferHandle scene_data_buffer = CreateBuffer(
            sizeof(GPUSceneData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            "scene data buffer"
        );
        // delete it next frame
        GetCurrentFrame().buffers_in_use.emplace_back(scene_data_buffer);

        glm::mat4 view =
            scene.frame_context.camera_rotation * glm::translate(scene.frame_context.camera_position);
        glm::mat4 projection = glm::perspective(
            glm::radians(scene.frame_context.camera_vertical_fov),
            (float)scene.draw_extent.width / (float)scene.draw_extent.height,
            10000.f,
            0.1f
        );

        // invert the Y direction on projection matrix so that we are more similar
        // to opengl and gltf axis
        projection[1][1] *= -1;

        GPUSceneData scene_data{};
        scene_data.view = view;
        scene_data.projection = projection;
        scene_data.view_projection = projection * view;
        scene_data.ambient_colour = scene.frame_context.ambient_colour;
        scene_data.light_colour = scene.frame_context.light_colour;
        scene_data.light_direction = scene.frame_context.light_direction;

        vmaCopyMemoryToAllocation(
            m_allocator, &scene_data, scene_data_buffer->allocation, 0, sizeof(scene_data)
        );

        // now we just need to bind it
        VkDescriptorSet scene_data_descriptor =
            GetCurrentFrame().frame_descriptors.Allocate(m_device_dispatch, m_scene_data_descriptor_layout);

        // update scene data descriptor
        Utils::DescriptorWriter writer{};
        writer.WriteBuffer(
            0, scene_data_buffer->buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        );
        writer.UpdateSet(m_device_dispatch, scene_data_descriptor);

        VkViewport viewport{};
        viewport.x = scene.viewport_position.x;
        viewport.y = scene.viewport_position.y;
        viewport.width = float(scene.draw_extent.width);
        viewport.height = float(scene.draw_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        m_device_dispatch.cmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{ 0, 0 };
        scissor.extent = scene.draw_extent;

        m_device_dispatch.cmdSetScissor(cmd, 0, 1, &scissor);

        VkRenderingAttachmentInfo color_attachment =
            Utils::AttachmentInfo(scene.draw_image->image_view, nullptr);

        VkClearValue clear_value{};
        clear_value.depthStencil.depth = 0.0f; // zero is far in reversed depth

        VkRenderingAttachmentInfo depth_attachment = Utils::AttachmentInfo(
            scene.depth_image->image_view, &clear_value, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
        );

        VkRenderingInfo render_info =
            Utils::RenderingInfo(&color_attachment, &depth_attachment, scene.draw_extent);

        m_device_dispatch.cmdBeginRendering(cmd, &render_info);

        for (const RenderObject& render_object : scene.frame_context.render_objects)
        {
            std::array<VkDescriptorSet, 2> sets{ scene_data_descriptor,
                                                 render_object.material->material_set };

            m_device_dispatch.cmdBindPipeline(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, render_object.material->pipeline->pipeline
            );

            m_device_dispatch.cmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                render_object.material->pipeline->layout,
                0,
                sets.size(),
                sets.data(),
                0,
                nullptr
            );

            GPUDrawPushConstants push_constants{};
            push_constants.opacity = 1.0f;
            push_constants.vertex_buffer_address = render_object.vertex_buffer_address;
            push_constants.world_matrix = render_object.transform;

            m_device_dispatch.cmdPushConstants(
                cmd,
                render_object.material->pipeline->layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(push_constants),
                &push_constants
            );

            m_device_dispatch.cmdBindIndexBuffer(cmd, render_object.index_buffer, 0, VK_INDEX_TYPE_UINT32);

            m_device_dispatch.cmdDrawIndexed(
                cmd, render_object.index_count, 1, render_object.first_index, 0, 0
            );
        }

        m_device_dispatch.cmdEndRendering(cmd);
    }

    void VulkanEngine::DrawImgui(VkCommandBuffer cmd, VkImageView target_image_view)
    {
        VkRenderingAttachmentInfo attachment_info = Utils::AttachmentInfo(target_image_view, nullptr);
        // swapchain extent because we're drawing directly onto it
        constexpr VkRenderingAttachmentInfo* depth_attachment_info = nullptr;
        VkRenderingInfo rendering_info =
            Utils::RenderingInfo(&attachment_info, depth_attachment_info, m_swapchain_extent);

        m_device_dispatch.cmdBeginRendering(cmd, &rendering_info);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        m_device_dispatch.cmdEndRendering(cmd);
    }

#pragma endregion Draw

#pragma region Init

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
        m_instance_dispatch =
            vkb::InstanceDispatchTable(vkb_instance.instance, vkb_instance.fp_vkGetInstanceProcAddr);
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
        features12.descriptorBindingSampledImageUpdateAfterBind = true;

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
        m_deletion_queue.PushFunction(
            "main surface",
            [this]()
            {
                m_instance_dispatch.destroySurfaceKHR(m_surface, nullptr);
            }
        );

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

        m_deletion_queue.PushFunction(
            "vmaAllocator",
            [this]()
            {
                vmaDestroyAllocator(m_allocator);
            }
        );
    }

    void VulkanEngine::InitCommands()
    {
        VkCommandPoolCreateInfo commandPoolInfo = Utils::CommandPoolCreateInfo(
            m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
        );

        for (std::size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            VK_CHECK(
                m_device_dispatch.createCommandPool(&commandPoolInfo, nullptr, &m_frames[i].command_pool)
            );

            VkCommandBufferAllocateInfo cmdAllocInfo =
                Utils::CommandBufferAllocateInfo(m_frames[i].command_pool, 1);

            VK_CHECK(m_device_dispatch.allocateCommandBuffers(&cmdAllocInfo, &m_frames[i].command_buffer));

            m_deletion_queue.PushFunction(
                "command pool",
                [i, this]()
                {
                    m_device_dispatch.destroyCommandPool(m_frames[i].command_pool, nullptr);
                }
            );
        }

        // immediate command buffer for short tasks
        VK_CHECK(m_device_dispatch.createCommandPool(&commandPoolInfo, nullptr, &m_immediate_command_pool));
        VkCommandBufferAllocateInfo cmdAllocInfo =
            Utils::CommandBufferAllocateInfo(m_immediate_command_pool, 1);
        VK_CHECK(m_device_dispatch.allocateCommandBuffers(&cmdAllocInfo, &m_immediate_command_buffer));
        m_deletion_queue.PushFunction(
            "Immediate command pool",
            [this]()
            {
                m_device_dispatch.destroyCommandPool(m_immediate_command_pool, nullptr);
            }
        );
    }

    void VulkanEngine::InitSyncStructures()
    {
        VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo(0);

        for (std::size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            VK_CHECK(m_device_dispatch.createFence(&fenceCreateInfo, nullptr, &m_frames[i].render_fence));

            m_deletion_queue.PushFunction(
                "fence",
                [i, this]()
                {
                    m_device_dispatch.destroyFence(m_frames[i].render_fence, nullptr);
                }
            );

            VK_CHECK(m_device_dispatch.createSemaphore(
                &semaphoreCreateInfo, nullptr, &m_frames[i].swapchain_semaphore
            ));
            VK_CHECK(m_device_dispatch.createSemaphore(
                &semaphoreCreateInfo, nullptr, &m_frames[i].render_semaphore
            ));

            m_deletion_queue.PushFunction(
                "semaphores x2",
                [i, this]()
                {
                    m_device_dispatch.destroySemaphore(m_frames[i].render_semaphore, nullptr);
                    m_device_dispatch.destroySemaphore(m_frames[i].swapchain_semaphore, nullptr);
                }
            );
        }

        // immediate command buffer for short tasks
        VK_CHECK(m_device_dispatch.createFence(&fenceCreateInfo, nullptr, &m_immediate_fence));
        m_deletion_queue.PushFunction(
            "Immediate fence",
            [this]()
            {
                m_device_dispatch.destroyFence(m_immediate_fence, nullptr);
            }
        );
    }

    void VulkanEngine::InitFrameDescriptors()
    {
        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            constexpr uint32_t frame_inital_sets = 32;
            // only uniform buffers for now
            std::vector<Utils::DescriptorPoolSizeRatio> sizes{ { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 } };
            m_frames[i].frame_descriptors.Init(m_device_dispatch, frame_inital_sets, sizes);
            m_deletion_queue.PushFunction(
                "frame descriptors",
                [i, this]()
                {
                    m_frames[i].frame_descriptors.DestroyPools(m_device_dispatch);
                }
            );
        }

        // create the layout
        Utils::DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // all stages maybe a bit heavy-handed but meh
        m_scene_data_descriptor_layout = builder.Build(m_device_dispatch, VK_SHADER_STAGE_ALL_GRAPHICS);

        m_deletion_queue.PushFunction(
            "scene descriptor layout",
            [this]()
            {
                m_device_dispatch.destroyDescriptorSetLayout(m_scene_data_descriptor_layout, nullptr);
            }
        );
    }

    void VulkanEngine::InitDefaultDescriptors() {}

    bool VulkanEngine::InitPipelines() { return InitMaterialPipelines(); }

    bool VulkanEngine::InitMaterialPipelines()
    {
        // create any materials (pipelines)
        m_gltf_pbr_material.BuildPipelines(m_material_interface);

        // allocator for materials
        if (m_gltf_pbr_material.loaded)
        {
            std::array<Utils::DescriptorPoolSizeRatio, 2> size_ratios{
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
            };
            m_gltf_pbr_material.descriptor_allocator.Init(m_device_dispatch, 1024, size_ratios);
        }

        m_deletion_queue.PushFunction(
            "pbr material",
            [this]()
            {
                m_gltf_pbr_material.DestroyResources(m_device_dispatch);
            }
        );

        return m_gltf_pbr_material.loaded;
    }

    void VulkanEngine::InitDefaultData()
    {
        // Create the default samplers
        VkSamplerCreateInfo sampler_create_info{};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.magFilter = VK_FILTER_NEAREST;
        sampler_create_info.minFilter = VK_FILTER_NEAREST;
        m_device_dispatch.createSampler(&sampler_create_info, nullptr, &m_default_sampler_nearest);
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        m_device_dispatch.createSampler(&sampler_create_info, nullptr, &m_default_sampler_linear);

        m_deletion_queue.PushFunction(
            "default samplers",
            [this]()
            {
                m_device_dispatch.destroySampler(m_default_sampler_nearest, nullptr);
                m_device_dispatch.destroySampler(m_default_sampler_linear, nullptr);
            }
        );

        // create the main scene
        glm::vec2 backbuffer_size{ m_window_extent.width, m_window_extent.height };
        backbuffer_size *= m_backbuffer_scale;

        Renderer::Scene& new_scene = render_scenes.emplace_back();
        new_scene.draw_image = CreateDrawImage((uint32_t)backbuffer_size.x, (uint32_t)backbuffer_size.y);
        new_scene.depth_image = CreateDepthImage((uint32_t)backbuffer_size.x, (uint32_t)backbuffer_size.y);
        new_scene.scene_name = "main scene";
        new_scene.render_scale = 1.0f;

        new_scene.frame_context.camera_position = glm::vec3(0.0f, 0.0f, -1.0f);
        new_scene.frame_context.camera_rotation = glm::mat4{ 1.0f }; // no rotation
        new_scene.frame_context.camera_vertical_fov = 70.0f;

        main_scene = 0;

        // load default textures
        // 3 default textures, white, black, grey. 1 pixel each
        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        m_white_image = AllocateImage(
            (void*)&white,
            VkExtent3D{ 1, 1, 1 },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            false,
            "white_image"
        );

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        m_black_image = AllocateImage(
            (void*)&black,
            VkExtent3D{ 1, 1, 1 },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            false,
            "black_image"
        );

        uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        m_grey_image = AllocateImage(
            (void*)&grey,
            VkExtent3D{ 1, 1, 1 },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            false,
            "grey_image"
        );

        // checkerboard image
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
        for (uint16_t x = 0; x < 16; x++)
        {
            for (uint16_t y = 0; y < 16; y++)
            {
                size_t idx = y * 16u + x;
                pixels[idx] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        m_checkerboard_image = AllocateImage(
            pixels.data(),
            VkExtent3D{ 16, 16, 1 },
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            false,
            "checkerboard_image"
        );

        // load testing mesh
        if (m_gltf_pbr_material.loaded)
        {
            Utils::LoadGltfIntoScene(
                render_scenes[main_scene], *this, "../data/resources/BarramundiFish.glb"
            );
        }
    }

    void VulkanEngine::InitImgui()
    {
        // Mostly copied from the examples
        VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                                              { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                              { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                              { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                              { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

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
            [](const char* function_name, void* engine)
            {
                VulkanEngine* engine_casted = reinterpret_cast<VulkanEngine*>(engine);
                return engine_casted->m_get_instance_proc_addr(engine_casted->m_instance, function_name);
            },
            this
        );

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

        m_deletion_queue.PushFunction(
            "imgui",
            [this, imgui_descriptor_pool]()
            {
                ImGui_ImplVulkan_Shutdown();
                m_device_dispatch.destroyDescriptorPool(imgui_descriptor_pool, nullptr);
            }
        );
    }

#pragma endregion Init

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
                    VkSurfaceFormatKHR{ .format = m_swapchain_format,
                                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
                )
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // #TODO: Implement MAILBOX present
                .set_desired_extent(width, height)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build()
                .value();

        m_swapchain_extent = vkb_swapchain.extent;
        m_swapchain = vkb_swapchain.swapchain;
        m_swapchain_images = vkb_swapchain.get_images().value();
        m_swapchain_image_views = vkb_swapchain.get_image_views().value();
    }

    ImageHandle VulkanEngine::CreateDrawImage(uint32_t width, uint32_t height)
    {
        VkExtent3D image_extent{ width, height, 1 };

        VkFormat image_format = VKENGINE_DRAW_IMAGE_FORMAT;
        VkImageUsageFlags usage_flags{};
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VkImageAspectFlagBits aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
        VkMemoryPropertyFlags required_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VmaAllocationCreateFlags allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        constexpr bool mipmapped = false;

        return AllocateImage(
            image_extent,
            image_format,
            usage_flags,
            memory_usage,
            aspect_flags,
            required_memory_flags,
            allocation_flags,
            mipmapped,
            "image_draw"
        );
    }

    ImageHandle VulkanEngine::CreateDepthImage(uint32_t width, uint32_t height)
    {
        VkExtent3D image_extent{ width, height, 1 };
        VkFormat image_format = VKENGINE_DEPTH_IMAGE_FORMAT;
        VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VkImageAspectFlagBits aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        VkMemoryPropertyFlags required_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VmaAllocationCreateFlags allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        constexpr bool mipmapped = false;

        return AllocateImage(
            image_extent,
            image_format,
            usage_flags,
            memory_usage,
            aspect_flags,
            required_memory_flags,
            allocation_flags,
            mipmapped,
            "image_depth"
        );
    }

    ImTextureID VulkanEngine::ImageDebugTextureId(const ImageHandle& image)
    {
        return reinterpret_cast<ImTextureID>(m_debug_image_map[image->image]);
    }

    void VulkanEngine::DestroySwapchain()
    {
        for (std::size_t i = 0; i < m_swapchain_image_views.size(); ++i)
        {
            m_device_dispatch.destroyImageView(m_swapchain_image_views[i], nullptr);
        }
        m_swapchain_image_views.clear();

        m_device_dispatch.destroySwapchainKHR(m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    void VulkanEngine::ResizeSwapchain()
    {
        m_device_dispatch.deviceWaitIdle();

        DestroySwapchain();

        int32_t width, height;
        SDL_GetWindowSize(m_window, &width, &height);
        if (width == 0 || height == 0)
        {
            // window is minimized, wait until we get a non-zero size
            return;
        }

        m_window_extent = VkExtent2D{ uint32_t(width), uint32_t(height) };
        CreateSwapchain(m_window_extent.width, m_window_extent.height);

        m_resize_requested = false;
    }

    void VulkanEngine::SetAllocationName(
        [[maybe_unused]] VmaAllocation allocation, [[maybe_unused]] const char* name
    )
    {
#ifdef CHEEKY_ENABLE_MEMORY_TRACKING
        vmaSetAllocationName(m_allocator, allocation, name);
#endif
    }
} // namespace Renderer