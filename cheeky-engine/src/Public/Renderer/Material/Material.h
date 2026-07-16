#pragma once

#include "GenericMaterial.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/VkTypes.h"

#include <VkBootstrapDispatch.h>
#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    class VulkanEngine;

    // Material type that supports (a subset of)glTF PBR specification.
    class GenericMaterial_GLTF_PBR
    {
      public:
        const char* VERTEX_SHADER = "data/shader/gltf_pbr.vert.spv";
        const char* FRAGMENT_SHADER = "data/shader/gltf_pbr.frag.spv";

        GenericMaterial_GLTF_PBR(MaterialEngineInterface& interface);
        ~GenericMaterial_GLTF_PBR();

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

        [[nodiscard]] bool IsValid() const { return m_material != std::nullopt; }

        std::optional<MaterialInstance> CreateInstance(
            vkb::DispatchTable& device_dispatch, MaterialPass pass, const Resources& resources
        );

      private:
        std::optional<GenericMaterial> m_material;

        // NOTE: the descriptor allocator is here because it's easier to manage this way.
        // this does however mean that the allocator will keep growing as we add more material instances
        // and will never shrink until the material is destroyed (likely during app destruction).
        // this is a leak. If it causes memory issues, make the allocator a resource and keep it reference
        // counted and stored in the engine instead.
        Utils::DescriptorAllocatorDynamic m_descriptor_allocator;
    };

    class GenericMaterial_Debug_Lines
    {
    public:
        const char* VERTEX_SHADER = "data/shader/debug_lines.vert.spv";
        const char* FRAGMENT_SHADER = "data/shader/debug_lines.frag.spv";

        GenericMaterial_Debug_Lines(MaterialEngineInterface& interface);
        ~GenericMaterial_Debug_Lines();

        [[nodiscard]] bool IsValid() const { return m_material != std::nullopt; }

        std::optional<MaterialInstance> CreateInstance(
            vkb::DispatchTable& device_dispatch, MaterialPass pass
        );

    private:
        std::optional<GenericMaterial> m_material;

        // NOTE: the descriptor allocator is here because it's easier to manage this way.
        // this does however mean that the allocator will keep growing as we add more material instances
        // and will never shrink until the material is destroyed (likely during app destruction).
        // this is a leak. If it causes memory issues, make the allocator a resource and keep it reference
        // counted and stored in the engine instead.
        Utils::DescriptorAllocatorDynamic m_descriptor_allocator;
    };
} // namespace Renderer