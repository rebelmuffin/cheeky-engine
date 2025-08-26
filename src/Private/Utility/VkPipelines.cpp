#include "Utility/VkPipelines.h"

#include "Utility/VkInitialisers.h"

#include <fstream>

namespace Utils
{
    bool LoadShaderModule(const vkb::DispatchTable& device_dispatch, const char* file_path,
                          VkShaderModule* out_shader_module)
    {
        // open with cursor at the end
        std::ifstream file(file_path, std::ios::ate | std::ios::binary);
        if (file.is_open() == false)
        {
            return false;
        }

        std::size_t file_size = std::size_t(file.tellg());
        // spirv expects uint32_t buffer
        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        // seek to file begin
        file.seekg(0);

        // load into buffer
        file.read((char*)buffer.data(), int64_t(file_size));

        file.close();

        VkShaderModuleCreateInfo shader_create_info{};
        shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_create_info.pNext = nullptr;
        shader_create_info.pCode = buffer.data();
        shader_create_info.codeSize = uint32_t(buffer.size() * sizeof(uint32_t));

        // check for success. Here we tolerate failures, don't wanna abort just because cannot load shader
        VkShaderModule shader_module;
        VkResult result = device_dispatch.createShaderModule(&shader_create_info, nullptr, &shader_module);
        if (result != VK_SUCCESS)
        {
            std::cout << "[!] Failed to load shader at: " << file_path << std::endl;
        }

        *out_shader_module = shader_module;
        return true;
    }

    PipelineBuilder::PipelineBuilder()
    {
        Clear();
    }

    void PipelineBuilder::Clear()
    {
        m_name = "";

        m_input_assembly = {};
        m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        m_rasteriser = {};
        m_rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        m_color_blend_attachment = {};

        m_multi_sampling = {};
        m_multi_sampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        m_pipeline_layout = {};

        m_depth_stencil = {};
        m_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        m_render_info = {};
        m_render_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

        m_stages.clear();
    }

    PipelineBuilder PipelineBuilder::SetName(const char* name)
    {
        m_name = name;
        return *this;
    }

    PipelineBuilder PipelineBuilder::SetLayout(VkPipelineLayout layout)
    {
        m_pipeline_layout = layout;
        return *this;
    }

    PipelineBuilder PipelineBuilder::AddVertexShader(VkShaderModule shader)
    {
        VkPipelineShaderStageCreateInfo stage_info =
            Utils::ShaderStageCreateInfo("main", shader, VK_SHADER_STAGE_VERTEX_BIT);
        m_stages.push_back(stage_info);

        return *this;
    }

    PipelineBuilder PipelineBuilder::AddFragmentShader(VkShaderModule shader)
    {
        VkPipelineShaderStageCreateInfo stage_info =
            Utils::ShaderStageCreateInfo("main", shader, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_stages.push_back(stage_info);

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
    {
        m_input_assembly.topology = topology;
        m_input_assembly.primitiveRestartEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
    {
        m_rasteriser.polygonMode = mode;
        m_rasteriser.lineWidth = 1.f;

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetCullMode(VkCullModeFlags cull_mode, VkFrontFace front_face)
    {
        m_rasteriser.cullMode = cull_mode;
        m_rasteriser.frontFace = front_face;

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetMultisamplingNone()
    {
        m_multi_sampling.sampleShadingEnable = VK_FALSE;
        m_multi_sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_multi_sampling.minSampleShading = 1.0f;
        m_multi_sampling.pSampleMask = nullptr;
        m_multi_sampling.alphaToCoverageEnable = VK_FALSE;
        m_multi_sampling.alphaToOneEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder PipelineBuilder::DisableBlending()
    {
        m_color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
        m_color_blend_attachment.blendEnable = VK_FALSE;

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
    {
        m_color_attachment_format = format;
        m_render_info.colorAttachmentCount = 1;
        m_render_info.pColorAttachmentFormats = &m_color_attachment_format;

        return *this;
    }

    PipelineBuilder PipelineBuilder::SetDepthFormat(VkFormat format)
    {
        m_render_info.depthAttachmentFormat = format;

        return *this;
    }

    PipelineBuilder PipelineBuilder::DisableDepthTest()
    {
        m_depth_stencil.depthTestEnable = VK_FALSE;
        m_depth_stencil.depthWriteEnable = VK_FALSE;
        m_depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        m_depth_stencil.depthBoundsTestEnable = VK_FALSE;
        m_depth_stencil.stencilTestEnable = VK_FALSE;
        m_depth_stencil.front = {};
        m_depth_stencil.back = {};
        m_depth_stencil.minDepthBounds = 0.0f;
        m_depth_stencil.maxDepthBounds = 1.0f;

        return *this;
    }

    VkPipeline PipelineBuilder::BuildPipeline(const vkb::DispatchTable& device_dispatch)
    {
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.pNext = nullptr;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.pNext = nullptr;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &m_color_blend_attachment;

        // we don't use this so it's okay to be empty.
        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.pNext = nullptr;

        VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_info{};
        dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_info.dynamicStateCount = sizeof(state) / sizeof(VkDynamicState);
        dynamic_info.pDynamicStates = state;

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.pNext = &m_render_info; // need to connect to render info for pipeline rendering
        pipeline_info.stageCount = uint32_t(m_stages.size());
        pipeline_info.pStages = m_stages.data();
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pDynamicState = &dynamic_info;
        pipeline_info.pInputAssemblyState = &m_input_assembly;
        pipeline_info.pRasterizationState = &m_rasteriser;
        pipeline_info.pMultisampleState = &m_multi_sampling;
        pipeline_info.pDepthStencilState = &m_depth_stencil;
        pipeline_info.layout = m_pipeline_layout;

        VkPipeline new_pipeline;
        VkResult result =
            device_dispatch.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline);
        if (result != VK_SUCCESS)
        {
            std::cout << "[!] Failed to create pipeline." << std::endl;
            return VK_NULL_HANDLE;
        }
        else
        {
            return new_pipeline;
        }
    }
} // namespace Utils