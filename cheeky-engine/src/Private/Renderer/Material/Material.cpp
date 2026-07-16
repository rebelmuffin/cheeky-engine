#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInterface.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/Utility/VkPipelines.h"
#include "Renderer/VkEngine.h"
#include "Renderer/VkTypes.h"

#include <array>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    GenericMaterial_GLTF_PBR::GenericMaterial_GLTF_PBR(MaterialEngineInterface& interface)
    {
        MaterialDefinition definition{};
        definition.frag_shader = FRAGMENT_SHADER;
        definition.vert_shader = VERTEX_SHADER;
        definition.descriptor.bindings.emplace_back(
            MaterialDescriptorBinding{ MaterialBindingType::UniformBuffer, sizeof(MaterialParameters) }
        );
        definition.descriptor.bindings.emplace_back(
            MaterialDescriptorBinding{ MaterialBindingType::ImageSampler }
        );
        definition.descriptor.bindings.emplace_back(
            MaterialDescriptorBinding{ MaterialBindingType::ImageSampler }
        );

        definition.pipelines[MaterialPass::MainColour] = MaterialPipelineDefinition{
            false, true, MaterialPipelinePolygonMode::Fill, MaterialPipelineTopology::Triangle
        };
        definition.pipelines[MaterialPass::Transparent] = MaterialPipelineDefinition{
            true, true, MaterialPipelinePolygonMode::Fill, MaterialPipelineTopology::Triangle
        };

        m_material = GenericMaterial::CreateMaterial("GLTF PBR", interface, definition);

        if (IsValid())
        {
            std::array<Utils::DescriptorPoolSizeRatio, 2> size_ratios{
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
            };
            m_descriptor_allocator.Init(*interface.device_dispatch_table, 1024, size_ratios);
        }
    }

    GenericMaterial_GLTF_PBR::~GenericMaterial_GLTF_PBR()
    {
        if (m_material.has_value())
        {
            m_material->DestroyResources();
            m_material.reset();
        }
    }

    std::optional<MaterialInstance> GenericMaterial_GLTF_PBR::CreateInstance(
        vkb::DispatchTable& device_dispatch, MaterialPass pass, const Resources& resources
    )
    {
        if (m_material == std::nullopt)
        {
            return std::nullopt;
        }

        GenericMaterialParameters params{};
        params.parameters.emplace_back(
            GenericMaterialParameter{ BufferParameter{ resources.uniform_buffer, resources.buffer_offset } }
        );
        params.parameters.emplace_back(
            GenericMaterialParameter{ ImageParameter{ resources.colour_image, resources.colour_sampler } }
        );
        params.parameters.emplace_back(
            GenericMaterialParameter{
                ImageParameter{ resources.metal_roughness_image, resources.metal_roughness_sampler } }
        );

        return m_material->CreateInstance(device_dispatch, pass, params, m_descriptor_allocator);
    }

    GenericMaterial_Debug_Lines::GenericMaterial_Debug_Lines(MaterialEngineInterface& interface)
    {
        MaterialDefinition definition{};
        definition.frag_shader = FRAGMENT_SHADER;
        definition.vert_shader = VERTEX_SHADER;

        // only main colour for debug lines, one depth and one without
        definition.pipelines[MaterialPass::MainColour] = MaterialPipelineDefinition{
            false, true, MaterialPipelinePolygonMode::Line, MaterialPipelineTopology::Line
        };

        definition.pipelines[MaterialPass::NoDepth] = MaterialPipelineDefinition{
            false, false, MaterialPipelinePolygonMode::Line, MaterialPipelineTopology::Line
        };

        m_material = GenericMaterial::CreateMaterial("GLTF PBR", interface, definition);

        if (IsValid())
        {
            std::array<Utils::DescriptorPoolSizeRatio, 2> size_ratios{
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                Utils::DescriptorPoolSizeRatio{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
            };
            m_descriptor_allocator.Init(*interface.device_dispatch_table, 1024, size_ratios);
        }
    }

    GenericMaterial_Debug_Lines::~GenericMaterial_Debug_Lines()
    {
        if (m_material.has_value())
        {
            m_material->DestroyResources();
            m_material.reset();
        }
    }

    std::optional<MaterialInstance> GenericMaterial_Debug_Lines::CreateInstance(
        vkb::DispatchTable& device_dispatch, MaterialPass pass
    )
    {
        if (m_material == std::nullopt)
        {
            return std::nullopt;
        }

        // no params for this material
        return m_material->CreateInstance(
            device_dispatch, pass, GenericMaterialParameters{}, m_descriptor_allocator
        );
    }

} // namespace Renderer