#include "Renderer/Material.h"
#include "Renderer/MaterialInterface.h"
#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/Utility/VkPipelines.h"
#include "Renderer/VkEngine.h"
#include "Renderer/VkTypes.h"

#include <array>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

namespace Renderer
{
    bool Material_GLTF_PBR::BuildPipelines(MaterialEngineInterface& interface)
    {
        Utils::DescriptorLayoutBuilder descriptor_layout_builder;
        descriptor_layout_builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // parameters
        descriptor_layout_builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // colour sampler
        descriptor_layout_builder.AddBinding(
            2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        ); // metal_roughness sampler

        descriptor_layout = descriptor_layout_builder.Build(
            *interface.device_dispatch_table, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
        );

        // create the layouts for each pipeline
        std::array<VkDescriptorSetLayout, 2> set_layouts{ interface.scene_data_descriptor_layout,
                                                          descriptor_layout };

        VkPushConstantRange range{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants) };

        // for all pipelines, 2 sets and 1 push constant
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.pSetLayouts = set_layouts.data();
        pipeline_layout_info.setLayoutCount = 2;
        pipeline_layout_info.pPushConstantRanges = &range;
        pipeline_layout_info.pushConstantRangeCount = 1;

        VkPipelineLayout layout;
        VkResult result =
            interface.device_dispatch_table->createPipelineLayout(&pipeline_layout_info, nullptr, &layout);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[!] Failed to create pipeline layout for glTF PBR material. Vulkan Error: "
                      << string_VkResult(result) << std::endl;
            return false;
        }

        // both share the layout
        opaque_pipeline.layout = layout;
        transparent_pipeline.layout = layout;

        // load in the shaders
        VkShaderModule frag_shader;
        if (Utils::LoadShaderModule(
                *interface.device_dispatch_table, "../data/shader/gltf_pbr.frag.spv", &frag_shader
            ) == false)
        {
            std::cerr << "[!] Failed to load glTF PBR fragment shader." << std::endl;
            return false;
        }

        VkShaderModule vert_shader;
        if (Utils::LoadShaderModule(
                *interface.device_dispatch_table, "../data/shader/gltf_pbr.vert.spv", &vert_shader
            ) == false)
        {
            std::cerr << "[!] Failed to load glTF PBR vertex shader." << std::endl;
            interface.device_dispatch_table->destroyShaderModule(frag_shader, nullptr);
            return false;
        }

        // create the pipelines!
        Utils::PipelineBuilder pipeline_builder =
            Utils::PipelineBuilder{}
                .SetLayout(layout)
                .AddFragmentShader(frag_shader)
                .AddVertexShader(vert_shader)
                .SetCullMode(
                    VK_CULL_MODE_BACK_BIT,
                    VK_FRONT_FACE_COUNTER_CLOCKWISE
                ) // idk why the meshes end up having counter clockwise tris
                .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .SetPolygonMode(VK_POLYGON_MODE_FILL)
                .SetColorAttachmentFormat(interface.draw_image_format)
                .SetDepthFormat(interface.depth_image_format)
                .EnableDepthTest(VK_COMPARE_OP_GREATER_OR_EQUAL) // greater or equal for inverse depth
                .SetMultisamplingNone()
                .DisableBlending(); // disabled for opaque one

        opaque_pipeline.pipeline = pipeline_builder.BuildPipeline(*interface.device_dispatch_table);

        pipeline_builder.EnableBlendingAlpha(); // alpha blending for transparent
        transparent_pipeline.pipeline = pipeline_builder.BuildPipeline(*interface.device_dispatch_table);

        // we can destroy the shader modules after building the pipelines

        interface.device_dispatch_table->destroyShaderModule(frag_shader, nullptr);
        interface.device_dispatch_table->destroyShaderModule(vert_shader, nullptr);

        loaded = true;

        return true;
    }

    void Material_GLTF_PBR::DestroyResources(vkb::DispatchTable& device_dispatch)
    {
        if (opaque_pipeline.pipeline != VK_NULL_HANDLE)
        {
            device_dispatch.destroyPipeline(opaque_pipeline.pipeline, nullptr);
            opaque_pipeline.pipeline = VK_NULL_HANDLE;
        }
        if (transparent_pipeline.pipeline != VK_NULL_HANDLE)
        {
            device_dispatch.destroyPipeline(transparent_pipeline.pipeline, nullptr);
            transparent_pipeline.pipeline = VK_NULL_HANDLE;
        }

        if (opaque_pipeline.layout != VK_NULL_HANDLE)
        {
            device_dispatch.destroyPipelineLayout(opaque_pipeline.layout, nullptr); // they share the layout
            opaque_pipeline.layout = VK_NULL_HANDLE;
            transparent_pipeline.layout = VK_NULL_HANDLE;
        }

        if (descriptor_layout != VK_NULL_HANDLE)
        {
            device_dispatch.destroyDescriptorSetLayout(descriptor_layout, nullptr);
        }
    }

    MaterialInstance Material_GLTF_PBR::CreateInstance(
        vkb::DispatchTable& device_dispatch,
        MaterialPass pass,
        const Resources& resources,
        Utils::DescriptorAllocatorDynamic& descriptor_allocator
    ) const
    {
        // create the material descriptor set
        VkDescriptorSet descriptor_set = descriptor_allocator.Allocate(device_dispatch, descriptor_layout);

        Utils::DescriptorWriter descriptor_writer{};

        // 0 is uniform buffer which is MaterialParameters
        descriptor_writer.WriteBuffer(
            0,
            resources.uniform_buffer,
            sizeof(MaterialParameters),
            resources.buffer_offset,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        );

        // 1 is colour image and 2 is metal_roughness image
        descriptor_writer.WriteImage(
            1,
            resources.colour_image.image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            resources.colour_sampler,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        );
        descriptor_writer.WriteImage(
            1,
            resources.metal_roughness_image.image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            resources.metal_roughness_sampler,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        );

        descriptor_writer.UpdateSet(device_dispatch, descriptor_set);

        const MaterialPipeline* pipeline;
        switch (pass)
        {
        case MaterialPass::Transparent:
            pipeline = &transparent_pipeline;
            break;
        case MaterialPass::MainColour:
            [[fallthrough]];
        case MaterialPass::Other:
            pipeline = &opaque_pipeline;
            break;
        }

        return MaterialInstance{ pipeline, descriptor_set, pass };
    }
} // namespace Renderer