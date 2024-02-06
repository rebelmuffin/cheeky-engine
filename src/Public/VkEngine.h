#pragma once

#include "VkTypes.h"

struct SDL_Window;

class VulkanEngine
{
  public:
    VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window, bool use_validation_layers);

    bool is_initialised{false};
    int frame_number{0};
    bool stop_rendering{false};

    // initializes everything in the engine
    void Init();

    // shuts down the engine
    void Cleanup();

    // run main loop
    void Update(double delta_ms);

  private:
    // draw loop
    void Draw(double delta_ms);

    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncStructures();

    void CreateSwapchain(uint32_t width, uint32_t height);
    void DestroySwapchain();

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkPhysicalDevice m_gpu;
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchain_format;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;
    VkExtent2D m_swapchain_extent;

    VkExtent2D m_window_extent;
    SDL_Window* m_window;

    bool m_use_validation_layers;

    uint64_t m_last_update_us = 0;
};