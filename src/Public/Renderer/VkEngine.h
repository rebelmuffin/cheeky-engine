#pragma once

#include "Renderer/Material.h"
#include "Renderer/MaterialInterface.h"
#include "Renderer/RenderObject.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/Scene.h"
#include "Renderer/Utility/DeletionQueue.h"
#include "Renderer/Utility/UploadRequest.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

#include <VkBootstrapDispatch.h>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

struct SDL_Window;

// we don't support having separate formats for scenes, these are unified across the engine.
constexpr VkFormat VKENGINE_DRAW_IMAGE_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat VKENGINE_DEPTH_IMAGE_FORMAT = VK_FORMAT_D32_SFLOAT;

namespace Renderer
{
    struct BackgroundPushConstants
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
        BackgroundPushConstants push_constants;
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

        // storage for reference counted handles so we can stop using them between frames.
        std::vector<BufferHandle> buffers_in_use;
        std::vector<ImageHandle> images_in_use;
    };

    constexpr int FRAME_OVERLAP = 2;

    class VulkanEngine
    {
      public:
        VulkanEngine(
            uint32_t window_width,
            uint32_t window_height,
            SDL_Window* window,
            float backbuffer_scale,
            bool use_validation_layers,
            bool immediate_uploads
        );
        VulkanEngine(const VulkanEngine&) = delete; // no copy pls

        bool is_initialised{ false };
        int frame_number{ 0 };
        bool stop_rendering{ false };

        // initializes everything in the engine
        bool Init();

        // shuts down the engine
        void Cleanup();

        // run main loop
        void Update(double delta_ms);

        std::vector<ComputeEffect>& ComputeEffects() { return m_compute_effects; }
        std::size_t CurrentComputeEffect() { return m_current_effect; }
        void SetCurrentComputeEffect(std::size_t target) { m_current_effect = target; }

        float GetRenderScale() const;
        void SetRenderScale(float scale);

        // these are used by things that write to the GPU memory like uploads.
        // can probably be interfaced to avoid making them public on the engine.
        FrameData& GetCurrentFrame() { return m_frames[frame_number % FRAME_OVERLAP]; }
        vkb::DispatchTable& DeviceDispatchTable() { return m_device_dispatch; }
        vkb::InstanceDispatchTable& InstanceDispatchTable() { return m_instance_dispatch; }
        VmaAllocator& Allocator() { return m_allocator; }
        Material_GLTF_PBR& PBRMaterial() { return m_gltf_pbr_material; }
        VkSampler Sampler() { return m_default_sampler_nearest; }
        ImageHandle PlaceholderImage() { return m_checkerboard_image; }
        ImageHandle WhiteImage() { return m_white_image; }
        ImageHandle BlackImage() { return m_black_image; }
        ImageHandle GreyImage() { return m_grey_image; }

        BufferHandle CreateBuffer(
            size_t allocation_size,
            VkBufferUsageFlags usage,
            VmaMemoryUsage memory_usage,
            VmaAllocationCreateFlags allocation_flags = 0,
            const char* debug_name = "unnamed_buffer"
        );
        BufferHandle CreateBuffer(
            void* buffer_data,
            size_t buffer_size,
            VkBufferUsageFlags usage,
            const char* debug_name = "unnamed_buffer"
        );
        void DestroyBuffer(const AllocatedBuffer& buffer);

        // allocate an empty image with given dimensions.
        ImageHandle AllocateImage(
            VkExtent3D image_extent,
            VkFormat format,
            VkImageUsageFlags usage,
            VmaMemoryUsage memory_usage,
            VkImageAspectFlagBits aspect_flags,
            VkMemoryPropertyFlags required_memory_flags = 0,
            VmaAllocationCreateFlags allocation_flags = 0,
            bool mipmapped = false,
            const char* debug_name = "unnamed_image"
        );

        // allocate an image and copy the given data inside. RGBA8 format is assumed.
        ImageHandle AllocateImage(
            void* image_data,
            VkExtent3D image_extent,
            VkFormat format,
            VkImageUsageFlags usage,
            bool mipmapped = false,
            const char* debug_name = "unnamed_image"
        );
        void DestroyImage(const AllocatedImage& image);

        GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
        MeshHandle RegisterMeshAsset(MeshAsset&& asset, std::string_view debug_name = "unnamed mesh");

        void RequestUpload(std::unique_ptr<Utils::IUploadRequest>&& upload_request);

        // helpers for creating scenes
        ImageHandle CreateDrawImage(uint32_t width, uint32_t height);
        ImageHandle CreateDepthImage(uint32_t width, uint32_t height);

        float test_mesh_opacity{ 1.0f };

        Scene* main_scene; // this is the scene that is rendered on the main window swapchain.
        std::vector<Scene> render_scenes;

      private:
        void DestroyPendingResources();
        void FinishPendingUploads(VkCommandBuffer cmd);
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

        // draw loop
        void Draw(double delta_ms);
        void DrawSceneBackground(const Scene& scene, VkCommandBuffer cmd);
        void DrawSceneGeometry(const Scene& scene, VkCommandBuffer cmd);
        void DrawImgui(VkCommandBuffer cmd, VkImageView target_image_view);

        bool InitVulkan();
        void InitAllocator();
        void InitCommands();
        void InitSyncStructures();
        void InitFrameDescriptors();
        void InitBackgroundDescriptors();
        void InitDefaultDescriptors();
        bool InitPipelines();
        bool InitBackgroundPipelines();
        bool InitMaterialPipelines();
        void InitDefaultData();
        void InitImgui();

        void CreateSwapchain(uint32_t width, uint32_t height);

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
        // there are functions that aren't implemented by vkb's dispatch tables. Need to lookup
        // manually :(
        PFN_vkGetDeviceProcAddr m_get_device_proc_addr;
        PFN_vkGetInstanceProcAddr m_get_instance_proc_addr;

        // default images used for debugging and fallback
        ImageHandle m_white_image;
        ImageHandle m_black_image;
        ImageHandle m_grey_image;
        ImageHandle m_checkerboard_image;

        // samplers we use for these
        VkSampler m_default_sampler_nearest;
        VkSampler m_default_sampler_linear;

        float m_backbuffer_scale;

        VkPipelineLayout m_gradient_pipeline_layout;
        std::vector<ComputeEffect> m_compute_effects{};
        std::size_t m_current_effect = 0;

        // test mesh and material instance
        std::shared_ptr<MeshAsset> m_default_mesh;
        MaterialInstance m_test_pbr_instance;
        BufferHandle m_test_pbr_uniform;

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

        VmaAllocator m_allocator;

        Utils::DescriptorAllocator m_background_descriptor_allocator;
        VkDescriptorSet m_background_compute_descriptors;
        VkDescriptorSetLayout m_background_compute_descriptor_layout;

        bool m_use_validation_layers;
        bool m_force_all_uploads_immediate;

        // uploads that are pending to be done on next frame.
        std::vector<std::unique_ptr<Utils::IUploadRequest>> m_pending_uploads;

        // uploads that have been completed this frame, but need to have their resources freed.
        std::vector<std::unique_ptr<Utils::IUploadRequest>> m_completed_uploads;
        Utils::DeletionQueue m_deletion_queue;

        uint64_t m_last_update_us = 0;
        bool m_resize_requested = false;

        VkDescriptorSetLayout m_scene_data_descriptor_layout;

        // materials (pipelines)
        Material_GLTF_PBR m_gltf_pbr_material;

        // interfaces
        MaterialEngineInterface m_material_interface;

        bool m_draw_resource_debugger = false;
        bool m_draw_engine_settings = false;
        bool m_draw_scene_editor = false;
        float m_camera_yaw_rad = 0.0f;
        float m_camera_pitch_rad = 0.0f;
        glm::vec3 m_camera_position{ 0.0f, 0.0f, -1.0f };
        bool m_rotating_camera = true;
        bool m_use_linear_sampling = true;

        // Resource storages. We manage the lifetime of all resources in the engine.
        ResourceStorage<AllocatedImage> m_image_storage;
        ResourceStorage<AllocatedBuffer> m_buffer_storage;
        ResourceStorage<MeshAsset> m_mesh_storage;
    };

} // namespace Renderer