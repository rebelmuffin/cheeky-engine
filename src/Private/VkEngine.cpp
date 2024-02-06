#include "VkEngine.h"
#include "Utility/VkImages.h"
#include "Utility/VkInitialisers.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>

#include <thread>

VulkanEngine::VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window,
                           bool use_validation_layers)
    : m_window_extent({window_width, window_height}), m_window(window), m_use_validation_layers(use_validation_layers)
{
}

void VulkanEngine::Init()
{
    InitVulkan();
    InitSwapchain();
    InitCommands();
    InitSyncStructures();
    is_initialised = true;
}

void VulkanEngine::Cleanup()
{
    vkDeviceWaitIdle(m_device);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);
        vkDestroyFence(m_device, m_frames[i].render_fence, nullptr);
        vkDestroySemaphore(m_device, m_frames[i].render_semaphore, nullptr);
        vkDestroySemaphore(m_device, m_frames[i].swapchain_semaphore, nullptr);
    }

    DestroySwapchain();
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
    vkDestroyInstance(m_instance, nullptr);
    is_initialised = false;
}

void VulkanEngine::Draw(double delta_ms)
{
    constexpr uint64_t one_second_ns = 1'000'000'000;
    VK_CHECK(vkWaitForFences(m_device, 1, &GetCurrentFrame().render_fence, true, one_second_ns));
    VK_CHECK(vkResetFences(m_device, 1, &GetCurrentFrame().render_fence));

    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, one_second_ns, GetCurrentFrame().swapchain_semaphore, nullptr,
                                   &swapchain_image_index));

    VkCommandBuffer cmd = GetCurrentFrame().command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    // COMMAND BEGIN

    Utils::TransitionImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clear_value;
    float flash = abs(sin(frame_number) / 120.0f);
    clear_value = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clear_range = Utils::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1,
                         &clear_range);

    Utils::TransitionImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // COMMAND END
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = Utils::CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo wait_info = Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                 GetCurrentFrame().swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info =
        Utils::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame().render_semaphore);

    VkSubmitInfo2 submit_info = Utils::SubmitInfo(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit_info, GetCurrentFrame().render_fence));

    VkPresentInfoKHR present_info =
        Utils::PresentInfo(&m_swapchain, &GetCurrentFrame().render_semaphore, &swapchain_image_index);
    VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

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

void VulkanEngine::InitVulkan()
{
    vkb::InstanceBuilder builder;

    vkb::Instance vkb_instance = builder.set_app_name("Vulkan Engine")
                                     .request_validation_layers(m_use_validation_layers)
                                     .use_default_debug_messenger()
                                     .require_api_version(1, 3, 0)
                                     .build()
                                     .value();

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

    m_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
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
}

void VulkanEngine::DestroySwapchain()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    for (size_t i = 0; i < m_swapchain_image_views.size(); ++i)
    {
        vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
    }
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
        VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].command_pool));

        VkCommandBufferAllocateInfo cmdAllocInfo = Utils::CommandBufferAllocateInfo(m_frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].command_buffer));
    }
}

void VulkanEngine::InitSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = Utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = Utils::SemaphoreCreateInfo(0);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].render_fence));

        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].render_semaphore));
    }
}