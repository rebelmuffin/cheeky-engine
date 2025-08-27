#pragma once

#include "Utility/DeletionQueue.h"
#include "Utility/VkDescriptors.h"
#include "VkBootstrapDispatch.h"
#include "VkTypes.h"
#include <vulkan/vulkan_core.h>

struct SDL_Window;

struct PushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

struct ComputeEffect
{
    const char* name;
    const char* path;
    VkPipeline pipeline;
    VkPipelineLayout layout;
    PushConstants push_constants;
};

struct FrameData
{
    VkCommandPool command_pool = nullptr;
    VkCommandBuffer command_buffer = nullptr;

    VkSemaphore swapchain_semaphore = nullptr; // so this frame waits for swapchain before rendering
    VkSemaphore render_semaphore = nullptr;    // so the present can wait for this frame to finish
    VkFence render_fence = nullptr;            // so we can wait for this frame on cpu
};

// The pending mesh upload structure is used to keep a list of pending uploads to execute on next draw.
// Staging buffer is deleted after this operation, can probably recycle it instead but eh.
struct PendingMeshUpload
{
    size_t vertex_buffer_size;
    size_t index_buffer_size;
    GPUMeshBuffers target_mesh;
    AllocatedBuffer staging_buffer;
};

constexpr int FRAME_OVERLAP = 2;

class VulkanEngine
{
  public:
    VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window, float backbuffer_scale,
                 bool use_validation_layers, bool immediate_uploads = false);

    bool is_initialised{false};
    int frame_number{0};
    bool stop_rendering{false};

    // initializes everything in the engine
    bool Init();

    // shuts down the engine
    void Cleanup();

    // run main loop
    void Update(double delta_ms);

    std::vector<ComputeEffect>& ComputeEffects()
    {
        return m_compute_effects;
    }
    std::size_t CurrentComputeEffect()
    {
        return m_current_effect;
    }
    void SetCurrentComputeEffect(std::size_t target)
    {
        m_current_effect = target;
    }

    AllocatedBuffer CreateBuffer(size_t allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void DestroyBuffer(const AllocatedBuffer& buffer);
    GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

  private:
    // draw loop
    void Draw(double delta_ms);
    void FinishPendingUploads(VkCommandBuffer cmd);
    void DrawBackground(VkCommandBuffer cmd);
    void DrawGeometry(VkCommandBuffer cmd);
    void DrawImgui(VkCommandBuffer cmd, VkImageView target_image_view);

    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    bool InitVulkan();
    void InitAllocator();
    void ResetSwapchain();
    void InitCommands();
    void InitSyncStructures();
    void InitDescriptors();
    bool InitPipelines();
    bool InitBackgroundPipelines();
    bool InitMeshPipeline();
    void InitDefaultData();
    void InitImgui();

    void CreateSwapchain(uint32_t width, uint32_t height);
    void CreateDrawImage();

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

    AllocatedImage m_draw_image;
    VkExtent2D m_draw_extent;
    VkDescriptorSet m_draw_image_descriptors;
    VkDescriptorSetLayout m_draw_image_descriptor_layout;
    float m_backbuffer_scale;

    VkPipelineLayout m_gradient_pipeline_layout;
    std::vector<ComputeEffect> m_compute_effects{};
    std::size_t m_current_effect = 0;

    VkPipeline m_mesh_pipeline;
    VkPipelineLayout m_mesh_pipeline_layout;
    GPUMeshBuffers m_rectangle_mesh;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;
    VkExtent2D m_swapchain_extent;

    FrameData m_frames[FRAME_OVERLAP];
    inline FrameData& GetCurrentFrame()
    {
        return m_frames[frame_number % FRAME_OVERLAP];
    }

    // immediate submit structures. For copying stuff to gpu
    VkFence m_immediate_fence;
    VkCommandBuffer m_immediate_command_buffer;
    VkCommandPool m_immediate_command_pool;

    VkQueue m_graphics_queue;
    uint32_t m_graphics_queue_family;

    VkExtent2D m_window_extent;
    SDL_Window* m_window;

    Utils::DescriptorAllocator m_descriptor_allocator;
    VmaAllocator m_allocator;

    bool m_use_validation_layers;
    bool m_immediate_uploads_enabled;

    std::vector<PendingMeshUpload> m_pending_uploads;
    Utils::DeletionQueue m_deletion_queue;

    uint64_t m_last_update_us = 0;
};