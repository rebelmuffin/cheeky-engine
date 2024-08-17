#pragma once

#include "Utility/DeletionQueue.h"
#include "VkBootstrapDispatch.h"
#include "VkTypes.h"

struct SDL_Window;

struct FrameData
{
    VkCommandPool command_pool = nullptr;
    VkCommandBuffer command_buffer = nullptr;

    VkSemaphore swapchain_semaphore = nullptr; // so this frame waits for swapchain before rendering
    VkSemaphore render_semaphore = nullptr;    // so the present can wait for this frame to finish
    VkFence render_fence = nullptr;            // so we can wait for this frame on cpu
};

constexpr int FRAME_OVERLAP = 2;

class VulkanEngine
{
  public:
    VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window, bool use_validation_layers);

    bool is_initialised{false};
    int frame_number{0};
    bool stop_rendering{false};

    // initializes everything in the engine
    bool Init();

    // shuts down the engine
    void Cleanup();

    // run main loop
    void Update(double delta_ms);

  private:
    // draw loop
    void Draw(double delta_ms);

    bool InitVulkan();
    void InitAllocator();
    void InitSwapchain();
    void InitCommands();
    void InitSyncStructures();

    void CreateSwapchain(uint32_t width, uint32_t height);

    void DeleteFence(int frame_idx);

    VkInstance m_instance = nullptr;
    VkDebugUtilsMessengerEXT m_debug_messenger = nullptr;
    VkPhysicalDevice m_gpu = nullptr;
    VkDevice m_device = nullptr;
    VkSurfaceKHR m_surface = nullptr;
    VkSwapchainKHR m_swapchain = nullptr;
    VkFormat m_swapchain_format;

    vkb::InstanceDispatchTable m_instance_dispatch;
    vkb::DispatchTable m_device_dispatch;
    // there are functions that aren't implemented by vkb's dispatch tables. Need to lookup manually :(
    PFN_vkGetDeviceProcAddr m_get_device_proc_addr;
    PFN_vkGetInstanceProcAddr m_get_instance_proc_addr;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;
    VkExtent2D m_swapchain_extent;

    FrameData m_frames[FRAME_OVERLAP];
    inline FrameData& GetCurrentFrame()
    {
        return m_frames[frame_number % FRAME_OVERLAP];
    }

    VkQueue m_graphics_queue;
    uint32_t m_graphics_queue_family;

    VkExtent2D m_window_extent;
    SDL_Window* m_window;

    VmaAllocator m_allocator;

    bool m_use_validation_layers;

    Utils::DeletionQueue m_deletion_queue;

    uint64_t m_last_update_us = 0;
};