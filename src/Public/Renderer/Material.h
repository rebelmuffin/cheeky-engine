#pragma once

#include "Renderer/MaterialInterface.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/VkTypes.h"
#include "VkBootstrapDispatch.h"
#include <glm/ext/vector_float4.hpp>
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
    };

    // Material type that supports (a subset of)glTF PBR specification.
    struct Material_GLTF_PBR
    {
        MaterialPipeline opaque_pipeline;
        MaterialPipeline transparent_pipeline;

        VkDescriptorSetLayout descriptor_layout;
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
            AllocatedImage colour_image;
            VkSampler colour_sampler;
            AllocatedImage metal_roughness_image;
            VkSampler metal_roughness_sampler;
            VkBuffer uniform_buffer;
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