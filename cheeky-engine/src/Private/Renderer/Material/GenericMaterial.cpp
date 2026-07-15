#include "Renderer/Material/GenericMaterial.h"

#include "Renderer/Material/MaterialInterface.h"
#include "Renderer/Utility/VkPipelines.h"
#include "Utilities/Log.h"

namespace
{
    VkDescriptorType ToVulkan(Renderer::MaterialBindingType binding_type)
    {
        switch (binding_type)
        {
        case Renderer::MaterialBindingType::ImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case Renderer::MaterialBindingType::UniformBuffer:
        default:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
    }

    VkPrimitiveTopology ToVulkan(Renderer::MaterialPipelineTopology topology)
    {
        switch (topology)
        {
        case Renderer::MaterialPipelineTopology::Line:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case Renderer::MaterialPipelineTopology::Triangle:
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkPolygonMode ToVulkan(Renderer::MaterialPipelinePolygonMode polygon_mode)
    {
        switch (polygon_mode)
        {
        case Renderer::MaterialPipelinePolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case Renderer::MaterialPipelinePolygonMode::Fill:
        default:
            return VK_POLYGON_MODE_FILL;
        }
    }

    std::string ToString(Renderer::MaterialBindingType type)
    {
        switch (type)
        {
        case Renderer::MaterialBindingType::ImageSampler:
            return "ImageSampler";
        case Renderer::MaterialBindingType::UniformBuffer:
            return "UniformBuffer";
        default:
            return "Unknown";
        }
    }

    std::string ToString(Renderer::MaterialPass pass)
    {
        switch (pass)
        {
        case Renderer::MaterialPass::MainColour:
            return "MainColour";
        case Renderer::MaterialPass::Transparent:
            return "Transparent";
        default:
            return "Unknown";
        }
    }
} // namespace

namespace Renderer
{
    GenericMaterial::GenericMaterial(
        std::string_view name, MaterialEngineInterface& interface, const MaterialDefinition& definition
    ) :

        m_name(name),
        m_device_dispatch(interface.device_dispatch_table),
        m_definition(definition),
        m_descriptor_layout(VK_NULL_HANDLE)
    {
    }

    std::optional<GenericMaterial> GenericMaterial::CreateMaterial(
        std::string_view name, MaterialEngineInterface& interface, const MaterialDefinition& definition
    )
    {
        GenericMaterial material{ name, interface, definition };
        const bool result = material.InitPipelines(interface);
        if (result == false)
        {
            return std::nullopt;
        }

        return std::optional{ material };
    }

    void GenericMaterial::DestroyResources()
    {
        if (m_descriptor_layout != VK_NULL_HANDLE)
        {
            m_device_dispatch->destroyDescriptorSetLayout(m_descriptor_layout, nullptr);
            m_descriptor_layout = VK_NULL_HANDLE;
        }
    }

    std::optional<MaterialInstance> GenericMaterial::CreateInstance(
        vkb::DispatchTable& device_dispatch,
        MaterialPass pass,
        const GenericMaterialParameters& params,
        Utils::DescriptorAllocatorDynamic& descriptor_allocator
    ) const
    {
        if (m_definition.descriptor.bindings.size() != params.parameters.size())
        {
            LOG_ERROR(
                "Trying to create a material instance with wrong number of parameters. "
                "Expected {} received {}",
                m_definition.descriptor.bindings.size(),
                params.parameters.size()
            );
            return std::nullopt;
        }

        const auto found_pipeline = m_pipelines.find(pass);
        if (found_pipeline == m_pipelines.end())
        {
            LOG_ERROR(
                "Trying to target a pass that does not exist. "
                "Material: {} Target: {}",
                m_name,
                ToString(pass)
            );
            return std::nullopt;
        }

        Utils::DescriptorWriter descriptor_writer{};
        bool errors = false;
        std::vector<BufferHandle> referenced_buffers{};
        std::vector<ImageHandle> referenced_images{};
        for (size_t i = 0; i < params.parameters.size(); ++i)
        {
            const GenericMaterialParameter& param = params.parameters[i];
            const MaterialDescriptorBinding& binding = m_definition.descriptor.bindings[i];
            if (const BufferParameter* buffer_param = std::get_if<BufferParameter>(&param.parameter))
            {
                if (binding.type != MaterialBindingType::UniformBuffer)
                {
                    LOG_ERROR(
                        "Wrong binding type in param {} for material {}, expected {} but received "
                        "UniformBuffer",
                        i,
                        m_name,
                        ToString(binding.type)
                    );
                    errors = true;
                    continue;
                }

                const size_t received_size = buffer_param->uniform_buffer->allocation_info.size;
                if (binding.size > received_size)
                {
                    LOG_ERROR(
                        "Uniform buffer size in param {} for material {} is wrong."
                        "Expected at least {} but received {}",
                        i,
                        m_name,
                        binding.size,
                        buffer_param->uniform_buffer->allocation_info.size
                    );
                    errors = true;
                    continue;
                }

                referenced_buffers.emplace_back(buffer_param->uniform_buffer);
                descriptor_writer.WriteBuffer(
                    i,
                    buffer_param->uniform_buffer->buffer,
                    binding.size,
                    buffer_param->buffer_offset,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                );
                continue;
            }

            if (const ImageParameter* image_param = std::get_if<ImageParameter>(&param.parameter))
            {
                if (binding.type != MaterialBindingType::ImageSampler)
                {
                    LOG_ERROR(
                        "Wrong binding type in param {} for material {}, expected {} but received "
                        "ImageSampler",
                        i,
                        m_name,
                        ToString(binding.type)
                    );
                    errors = true;
                    continue;
                }

                referenced_images.emplace_back(image_param->image);
                descriptor_writer.WriteImage(
                    i,
                    image_param->image->image_view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    image_param->sampler,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                );
            }
        }

        if (errors)
        {
            return std::nullopt;
        }

        // create and write to the set
        const VkDescriptorSet descriptor_set =
            descriptor_allocator.Allocate(device_dispatch, m_descriptor_layout);
        descriptor_writer.UpdateSet(device_dispatch, descriptor_set);

        return MaterialInstance{
            &m_pipelines.at(pass), descriptor_set, pass, referenced_images, referenced_buffers
        };
    }

    bool GenericMaterial::InitPipelines(MaterialEngineInterface& interface)
    {
        Utils::DescriptorLayoutBuilder descriptor_layout_builder{};
        for (size_t i = 0; i < m_definition.descriptor.bindings.size(); i++)
        {
            const MaterialDescriptorBinding& binding = m_definition.descriptor.bindings[i];
            VkDescriptorType type = ToVulkan(binding.type);
            descriptor_layout_builder.AddBinding(i, type);
        }

        m_descriptor_layout = descriptor_layout_builder.Build(
            *interface.device_dispatch_table, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
        );

        // load in the shaders
        VkShaderModule frag_shader;
        if (Utils::LoadShaderModule(
                *interface.device_dispatch_table, m_definition.frag_shader.data(), &frag_shader
            ) == false)
        {
            LOG_ERROR("[!] Failed to load glTF PBR fragment shader at {}", m_definition.frag_shader.data());
            DestroyResources();
            return false;
        }

        VkShaderModule vert_shader;
        if (Utils::LoadShaderModule(
                *interface.device_dispatch_table, m_definition.vert_shader.data(), &vert_shader
            ) == false)
        {
            LOG_ERROR("[!] Failed to load glTF PBR vertex shader at {}", m_definition.vert_shader.data());
            interface.device_dispatch_table->destroyShaderModule(frag_shader, nullptr);
            DestroyResources();
            return false;
        }

        // Create the pipeline layout and the pipelines themselves

        // one set layout per pipeline
        std::array<VkDescriptorSetLayout, 2> set_layouts{ interface.scene_data_descriptor_layout,
                                                          m_descriptor_layout };

        // always the same push constants
        VkPushConstantRange push_constant_range{ VK_SHADER_STAGE_VERTEX_BIT,
                                                 0,
                                                 sizeof(GPUDrawPushConstants) };

        // for all pipelines, 2 sets and 1 push constant
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.pSetLayouts = set_layouts.data();
        pipeline_layout_info.setLayoutCount = 2;
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;
        pipeline_layout_info.pushConstantRangeCount = 1;

        VkPipelineLayout layout;
        VkResult result =
            interface.device_dispatch_table->createPipelineLayout(&pipeline_layout_info, nullptr, &layout);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR(
                "[!] Failed to create pipeline layout for glTF PBR material. Vulkan Error: ",
                string_VkResult(result)
            );
            DestroyResources();
            return false;
        }

        // base settings
        Utils::PipelineBuilder pipeline_builder =
            Utils::PipelineBuilder{}
                .SetLayout(layout)
                .AddFragmentShader(frag_shader)
                .AddVertexShader(vert_shader)
                .SetCullMode(
                    VK_CULL_MODE_BACK_BIT,
                    VK_FRONT_FACE_COUNTER_CLOCKWISE
                ) // idk why the meshes end up having counter clockwise tris
                .SetColorAttachmentFormat(interface.draw_image_format)
                .SetMultisamplingNone();

        for (const auto& [pass, definition] : m_definition.pipelines)
        {
            pipeline_builder.SetInputTopology(ToVulkan(definition.topology));
            pipeline_builder.SetPolygonMode(ToVulkan(definition.polygon_mode));

            if (definition.enable_depth)
            {
                pipeline_builder.SetDepthFormat(interface.depth_image_format)
                    .EnableDepthTest(VK_COMPARE_OP_GREATER_OR_EQUAL); // greater or equal for inverse depth
            }
            else
            {
                pipeline_builder.DisableDepthTest();
            }

            if (definition.enable_transparency)
            {
                pipeline_builder.EnableBlendingAlpha();
            }
            else
            {
                pipeline_builder.DisableBlending();
            }

            m_pipelines[pass] = { pipeline_builder.BuildPipeline(*interface.device_dispatch_table), layout };
        }

        interface.device_dispatch_table->destroyShaderModule(frag_shader, nullptr);
        interface.device_dispatch_table->destroyShaderModule(vert_shader, nullptr);

        return true;
    }
} // namespace Renderer