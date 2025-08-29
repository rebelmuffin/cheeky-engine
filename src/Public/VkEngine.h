#pragma once

#include "Utility/DeletionQueue.h"
#include "Utility/UploadRequest.h"
#include "Utility/VkDescriptors.h"
#include "Utility/VkLoader.h"
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

    Utils::DescriptorAllocatorDynamic frame_descriptors;
    Utils::DeletionQueue deletion_queue;
};

constexpr int FRAME_OVERLAP = 2;

class VulkanEngine
{
  public:
    VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window, float backbuffer_scale,
                 bool use_validation_layers, bool immediate_uploads);
    VulkanEngine(const VulkanEngine&) = delete; // no copy pls

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

    float GetRenderScale() const;
    void SetRenderScale(float scale);

    // these are used by things that write to the GPU memory like uploads.
    // can probably be interfaced to avoid making them public on the engine.
    FrameData& GetCurrentFrame()
    {
        return m_frames[frame_number % FRAME_OVERLAP];
    }
    vkb::DispatchTable& DeviceDispatchTable()
    {
        return m_device_dispatch;
    }
    vkb::InstanceDispatchTable& InstanceDispatchTable()
    {
        return m_instance_dispatch;
    }

    AllocatedBuffer CreateBuffer(size_t allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                                 const char* debug_name = "unnamed_buffer");
    void DestroyBuffer(const AllocatedBuffer& buffer);
    GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
    AllocatedImage AllocateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                                 VmaMemoryUsage memory_usage, VkImageAspectFlagBits aspect_flags,
                                 VkMemoryPropertyFlags additional_flags = 0, const char* debug_name = "unnamed_image");
    void RequestUpload(std::unique_ptr<Utils::IUploadRequest>&& upload_request);

    float test_mesh_opacity{1.0f};

  private:
    // draw loop
    void Draw(double delta_ms);
    void DrawBackground(VkCommandBuffer cmd);
    void DrawGeometry(VkCommandBuffer cmd);
    void DrawImgui(VkCommandBuffer cmd, VkImageView target_image_view);

    void FinishPendingUploads(VkCommandBuffer cmd);
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    bool InitVulkan();
    void InitAllocator();
    void InitCommands();
    void InitSyncStructures();
    void InitFrameDescriptors();
    void InitBackgroundDescriptors();
    bool InitPipelines();
    bool InitBackgroundPipelines();
    bool InitMeshPipeline();
    void InitDefaultData();
    void InitImgui();

    void CreateSwapchain(uint32_t width, uint32_t height);
    void CreateDrawImage();
    void CreateDepthImage();

    void DestroySwapchain();
    void ResizeSwapchain();
    void SetAllocationName(VmaAllocation allocation, const char* name);

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
    AllocatedImage m_depth_image;

    VkExtent2D m_draw_extent;
    float m_render_scale = 1.0f;
    VkDescriptorSet m_background_compute_descriptors;
    VkDescriptorSetLayout m_background_compute_descriptor_layout;
    float m_backbuffer_scale;

    VkPipelineLayout m_gradient_pipeline_layout;
    std::vector<ComputeEffect> m_compute_effects{};
    std::size_t m_current_effect = 0;

    VkPipeline m_mesh_pipeline;
    VkPipelineLayout m_mesh_pipeline_layout;
    std::shared_ptr<MeshAsset> m_default_mesh;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;
    VkExtent2D m_swapchain_extent;

    FrameData m_frames[FRAME_OVERLAP];

    // immediate submit structures. For copying stuff to gpu
    VkFence m_immediate_fence;
    VkCommandBuffer m_immediate_command_buffer;
    VkCommandPool m_immediate_command_pool;

    VkQueue m_graphics_queue;
    uint32_t m_graphics_queue_family;

    VkExtent2D m_window_extent;
    SDL_Window* m_window;

    Utils::DescriptorAllocator m_background_descriptor_allocator;
    VmaAllocator m_allocator;

    bool m_use_validation_layers;
    bool m_force_all_uploads_immediate;

    // uploads that are pending to be done on next frame.
    std::vector<std::unique_ptr<Utils::IUploadRequest>> m_pending_uploads;

    // uploads that have been completed this frame, but need to have their resources freed.
    std::vector<std::unique_ptr<Utils::IUploadRequest>> m_completed_uploads;
    Utils::DeletionQueue m_deletion_queue;

    uint64_t m_last_update_us = 0;
    bool m_resize_requested = false;

    GPUSceneData m_scene_data; // this is the scene data that is uploaded each frame.
    VkDescriptorSetLayout m_scene_data_descriptor_layout;

    bool m_draw_engine_settings = false;
    float m_camera_yaw_rad = 0.0f;
    float m_camera_pitch_rad = 0.0f;
    glm::vec3 m_camera_position{0.0f, 0.0f, -1.0f};
    bool m_rotating_camera = true;
};