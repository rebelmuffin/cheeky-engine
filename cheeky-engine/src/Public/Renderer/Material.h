#pragma once

#include "Renderer/MaterialInterface.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/VkTypes.h"
#include "VkBootstrapDispatch.h"
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace Renderer
{
    class VulkanEngine;

    enum class MaterialPass : uint8_t
    {
        MainColour,
        Transparent,
        Other
    };

    struct MaterialPipeline
    {
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    struct MaterialInstance
    {
        const MaterialPipeline* pipeline;
        VkDescriptorSet material_set;
        MaterialPass pass;

        // keep a list of handles around to make sure the referenced resources don't get deleted mid-use
        std::vector<ImageHandle> referenced_images;
        std::vector<BufferHandle> referenced_buffers;
    };

    // Material type that supports (a subset of)glTF PBR specification.
    struct Material_GLTF_PBR
    {
        MaterialPipeline opaque_pipeline;
        MaterialPipeline transparent_pipeline;

        VkDescriptorSetLayout descriptor_layout;

        // NOTE: the descriptor allocator is here beacause it's easier to manage this way.
        // this does however mean that the allocator will keep growing as we add more material instances
        // and will never shrink until the material is destroyed (likely during app destruction).
        // this is a leak. If it causes memory issues, make the allocator a resource and keep it reference
        // counted and stored in the engine instead.
        Utils::DescriptorAllocatorDynamic descriptor_allocator;
        bool loaded = false;

        // This is what gets written into the uniform buffer
        struct MaterialParameters
        {
            glm::vec4 colour;
            glm::vec4 metal_roughness;

            // padding for extra crap later
            glm::vec4 extra[14];
        };

        // These are the resources required to draw a single instance of this material
        struct Resources
        {
            ImageHandle colour_image;
            VkSampler colour_sampler;
            ImageHandle metal_roughness_image;
            VkSampler metal_roughness_sampler;
            BufferHandle uniform_buffer;
            uint32_t buffer_offset;
        };

        bool BuildPipelines(MaterialEngineInterface& interface);
        void DestroyResources(vkb::DispatchTable& device_dispatch);

        // create a material instance that can be used to render objects using the given resources.
        MaterialInstance CreateInstance(
            vkb::DispatchTable& device_dispatch,
            MaterialPass pass,
            const Resources& resources,
            Utils::DescriptorAllocatorDynamic& descriptor_allocator
        ) const;
    };
} // namespace Renderer