#include "VkEngine.h"
#include "Utility/VkImages.h"
#include "Utility/VkInitialisers.h"
#include "Utility/VkPipelines.h"
#include "VkThirdParty.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>

#include <cstdint>
#include <iostream>
#include <thread>

#define VK_DEVICE_CALL(device, function, ...)                                                                          \
    reinterpret_cast<PFN_##function>(m_get_device_proc_addr(device, #function))(device, __VA_ARGS__);

#define VK_INSTANCE_CALL(instance, function, ...)                                                                      \
    reinterpret_cast<PFN_##function>(m_get_instance_proc_addr(instance, #function))(instance, __VA_ARGS__);

VulkanEngine::VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window,
                           bool use_validation_layers)
    : m_window_extent({window_width, window_height}), m_window(window), m_use_validation_layers(use_validation_layers)
{
}

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

    uint32_t swapchain_image_index;
    VkResult e = m_device_dispatch.acquireNextImageKHR(
        m_swapchain, one_second_ns, GetCurrentFrame().swapchain_semaphore, nullptr, &swapchain_image_index);
    if (e == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ResetSwapchain();
        return;
    }

    VK_CHECK(m_device_dispatch.resetFences(1, &GetCurrentFrame().render_fence));

    VkCommandBuffer cmd = GetCurrentFrame().command_buffer;
    VK_CHECK(m_device_dispatch.resetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(m_device_dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
    // COMMAND BEGIN

    Utils::TransitionImage(&m_device_dispatch, cmd, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);
    DrawBackground(cmd);
    Utils::TransitionImage(&m_device_dispatch, cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // copy draw into swapchain
    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    Utils::CopyImageToImage(&m_device_dispatch, cmd, m_draw_image.image, m_swapchain_images[swapchain_image_index],
                            m_draw_extent, m_swapchain_extent);
    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
    VK_CHECK(m_device_dispatch.queuePresentKHR(m_graphics_queue, &present_info));

    ++frame_number;
}

void VulkanEngine::DrawBackground(VkCommandBuffer cmd)
{
    m_device_dispatch.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradient_pipeline);
    m_device_dispatch.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradient_pipeline_layout, 0, 1,
                                            &m_draw_image_descriptors, 0, nullptr);
    m_device_dispatch.cmdDispatch(cmd, uint32_t(std::ceil(float(m_draw_extent.width) / 16.0f)),
                                  uint32_t(std::ceil(float(m_draw_extent.height) / 16.0f)), 1);
}

void VulkanEngine::Update(double delta_ms)
{
    if (stop_rendering)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    Draw(delta_ms);
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

    for (size_t i = 0; i < m_swapchain_image_views.size(); ++i)
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
    // destroy in case already exists
    m_device_dispatch.destroyImageView(m_draw_image.image_view, nullptr);
    vmaDestroyImage(m_allocator, m_draw_image.image, m_draw_image.allocation);

    VkExtent3D image_extent{m_window_extent.width, m_window_extent.height, 1};

    m_draw_extent = VkExtent2D{image_extent.width, image_extent.height};
    m_draw_image.image_extent = image_extent;
    m_draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcoded to 32bit float

    VkImageUsageFlags usage_flags{};
    usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo image_info =
        Utils::ImageCreateInfo(m_draw_image.image_format, usage_flags, m_draw_image.image_extent);

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocation_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(m_allocator, &image_info, &allocation_info, &m_draw_image.image, &m_draw_image.allocation, nullptr);

    // Image is created! Now just need a view for it
    VkImageViewCreateInfo image_view_info =
        Utils::ImageViewCreateInfo(m_draw_image.image_format, m_draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(m_device_dispatch.createImageView(&image_view_info, nullptr, &m_draw_image.image_view));

    m_deletion_queue.PushFunction("draw image", [this]() {
        m_device_dispatch.destroyImageView(m_draw_image.image_view, nullptr);
        vmaDestroyImage(m_allocator, m_draw_image.image, m_draw_image.allocation);
    });
}

void VulkanEngine::ResetSwapchain()
{
    CreateSwapchain(m_window_extent.width, m_window_extent.height);
    CreateDrawImage();
}

void VulkanEngine::InitCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo =
        Utils::CommandPoolCreateInfo(m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(m_device_dispatch.createCommandPool(&commandPoolInfo, nullptr, &m_frames[i].command_pool));

        VkCommandBufferAllocateInfo cmdAllocInfo = Utils::CommandBufferAllocateInfo(m_frames[i].command_pool, 1);

        VK_CHECK(m_device_dispatch.allocateCommandBuffers(&cmdAllocInfo, &m_frames[i].command_buffer));

        m_deletion_queue.PushFunction(
            "command pool", [i, this]() { m_device_dispatch.destroyCommandPool(m_frames[i].command_pool, nullptr); });
    }
}

void VulkanEngine::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo(0);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
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
    return InitBackgroundPipelines();
}

bool VulkanEngine::InitBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;
    layout_info.pSetLayouts = &m_draw_image_descriptor_layout;
    layout_info.setLayoutCount = 1;

    VK_CHECK(m_device_dispatch.createPipelineLayout(&layout_info, nullptr, &m_gradient_pipeline_layout));

    VkShaderModule shader_module{};
    if (Utils::LoadShaderModule(m_device_dispatch, "../data/shader/gradient.comp.spv", &shader_module) == false)
    {
        m_device_dispatch.destroyPipelineLayout(m_gradient_pipeline_layout, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext = nullptr;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = shader_module;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo compute_pipeline_info{};
    compute_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_info.pNext = nullptr;
    compute_pipeline_info.layout = m_gradient_pipeline_layout;
    compute_pipeline_info.stage = stage_info;

    VK_CHECK(m_device_dispatch.createComputePipelines(VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr,
                                                      &m_gradient_pipeline));

    // Clean up
    m_device_dispatch.destroyShaderModule(shader_module, nullptr);
    m_deletion_queue.PushFunction("pipelines", [this]() {
        m_device_dispatch.destroyPipelineLayout(m_gradient_pipeline_layout, nullptr);
        m_device_dispatch.destroyPipeline(m_gradient_pipeline, nullptr);
    });

    return true;
}