#include "Renderer/Swapchain.h"

#include "Renderer/Utility/VkInitialisers.h"
#include "VkBootstrap.h"
#include "VkBootstrapDispatch.h"

namespace Renderer
{
    Swapchain::Swapchain(const vkb::DispatchTable& device_dispatch) : m_device_dispatch(&device_dispatch) {}
    Swapchain::~Swapchain() { Destroy(); }

    void Swapchain::Create(uint32_t width, uint32_t height, VkPhysicalDevice gpu, VkSurfaceKHR surface)
    {
        VK_CHECK(m_device_dispatch->deviceWaitIdle());

        Destroy();

        vkb::SwapchainBuilder builder(gpu, m_device_dispatch->device, surface);

        m_format = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain vkb_swapchain =
            builder
                .set_desired_format(
                    VkSurfaceFormatKHR{ .format = m_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
                )
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // #TODO: Implement MAILBOX present
                .set_desired_extent(width, height)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build()
                .value();

        m_extent = vkb_swapchain.extent;
        m_swapchain = vkb_swapchain.swapchain;

        // BEWARE: this has to be called once. Calling GET_IMAGE_VIEWS actually allocates new image views
        //         every time, cool interface bro
        std::vector<VkImageView> swapchain_image_views = vkb_swapchain.get_image_views().value();
        std::vector<VkImage> swapchain_images = vkb_swapchain.get_images().value();
        m_swapchain_images.resize(vkb_swapchain.image_count);
        VkSemaphoreCreateInfo semaphore_create_info = Utils::SemaphoreCreateInfo(0);
        for (uint32_t i = 0; i < vkb_swapchain.image_count; i++)
        {
            VkSemaphore semaphore{};
            VK_CHECK(m_device_dispatch->createSemaphore(&semaphore_create_info, nullptr, &semaphore));

            m_swapchain_images[i] =
                SwapchainImage{ swapchain_images[i], swapchain_image_views[i], semaphore };

#ifdef CHEEKY_ENABLE_MEMORY_TRACKING
            VkDebugUtilsObjectNameInfoEXT nameInfo{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
                .objectHandle = (uint64_t)m_swapchain_images[i].view,
                .pObjectName = "Swapchain Image View",
            };

            VK_CHECK(m_device_dispatch->setDebugUtilsObjectNameEXT(&nameInfo));
#endif
        }

        m_created = true;
    }

    void Swapchain::Destroy()
    {
        for (SwapchainImage& img : m_swapchain_images)
        {
            m_device_dispatch->destroyImageView(img.view, nullptr);
            m_device_dispatch->destroySemaphore(img.queue_submit_semaphore, nullptr);
        }
        if (m_swapchain != VK_NULL_HANDLE)
        {
            m_device_dispatch->destroySwapchainKHR(m_swapchain, nullptr);
        }

        m_swapchain_images.clear();
        m_extent = {};
        m_created = false;
    }

    SwapchainImage* Swapchain::GetSwapchainImage(uint32_t idx)
    {
        if (m_created == false || m_swapchain_images.empty())
        {
            return nullptr;
        }

        return &m_swapchain_images[idx];
    }
} // namespace Renderer