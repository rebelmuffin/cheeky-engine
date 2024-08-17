#include "VkEngine.h"
#include "Utility/VkImages.h"
#include "Utility/VkInitialisers.h"

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

    InitSwapchain();
    InitCommands();
    InitSyncStructures();

    return true;
}

void VulkanEngine::Cleanup()
{
    m_device_dispatch.deviceWaitIdle();

    for (uint64_t i = m_deletion_queue.size() - 1; i > 0; --i)
    {
        m_deletion_queue[i]();
    }

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
    VK_CHECK(m_device_dispatch.acquireNextImageKHR(m_swapchain, one_second_ns, GetCurrentFrame().swapchain_semaphore,
                                                   nullptr, &swapchain_image_index));

    VkCommandBuffer cmd = GetCurrentFrame().command_buffer;
    VK_CHECK(m_device_dispatch.resetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(m_device_dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
    // COMMAND BEGIN

    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index],
                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clear_value;
    float flash = abs(sin(float(frame_number) / 120.f));
    clear_value = {{flash * 0.2f, flash * 0.2f, flash * 0.8f, 1.0f}};

    VkImageSubresourceRange clear_range = Utils::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    m_device_dispatch.cmdClearColorImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL,
                                         &clear_value, 1, &clear_range);

    Utils::TransitionImage(&m_device_dispatch, cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
    m_deletion_queue.push_back([this]() { m_instance_dispatch.destroySurfaceKHR(m_surface, nullptr); });

    return true;
}

void VulkanEngine::CreateSwapchain(uint32_t width, uint32_t height)
{
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
        m_deletion_queue.push_back(
            [i, this]() { m_device_dispatch.destroyImageView(m_swapchain_image_views[i], nullptr); });
    }
    m_deletion_queue.push_back([this]() { m_device_dispatch.destroySwapchainKHR(m_swapchain, nullptr); });
}

void VulkanEngine::InitSwapchain()
{
    CreateSwapchain(m_window_extent.width, m_window_extent.height);
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

        m_deletion_queue.push_back(
            [i, this]() { m_device_dispatch.destroyCommandPool(m_frames[i].command_pool, nullptr); });
    }
}

void VulkanEngine::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo(0);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(m_device_dispatch.createFence(&fenceCreateInfo, nullptr, &m_frames[i].render_fence));

        m_deletion_queue.push_back([i, this]() { m_device_dispatch.destroyFence(m_frames[i].render_fence, nullptr); });

        VK_CHECK(m_device_dispatch.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frames[i].swapchain_semaphore));
        VK_CHECK(m_device_dispatch.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frames[i].render_semaphore));

        m_deletion_queue.push_back([i, this]() {
            m_device_dispatch.destroySemaphore(m_frames[i].render_semaphore, nullptr);
            m_device_dispatch.destroySemaphore(m_frames[i].swapchain_semaphore, nullptr);
        });
    }
}