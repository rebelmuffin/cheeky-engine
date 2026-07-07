#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace vkb
{
    struct DispatchTable;
}

namespace Renderer
{
    struct SwapchainImage
    {
        VkImage image;
        VkImageView view;
        VkSemaphore queue_submit_semaphore;
    };

    class Swapchain
    {
      public:
        Swapchain(const vkb::DispatchTable& device_dispatch);
        ~Swapchain();

        /*
         * Create the swapchain resources. If the swapchain is live, will destroy the existing resources
         * first.
         */
        void Create(uint32_t width, uint32_t height, VkPhysicalDevice gpu, VkSurfaceKHR surface);
        void Destroy();

        VkSwapchainKHR& VkSwapchain() { return m_swapchain; }
        const VkFormat& Format() const { return m_format; }
        const VkExtent2D& Extent() const { return m_extent; }
        SwapchainImage* GetSwapchainImage(uint32_t idx);

      private:
        const vkb::DispatchTable* m_device_dispatch;

        bool m_created = false;
        VkExtent2D m_extent{};
        VkFormat m_format{};
        VkSwapchainKHR m_swapchain{};
        std::vector<SwapchainImage> m_swapchain_images{};
    };
} // namespace Renderer
