#pragma once

#include "Renderer/VkTypes.h"

#include <VkBootstrapDispatch.h>

namespace Renderer::Utils
{
    bool LoadShaderModule(const vkb::DispatchTable& device_dispatch, const char* file_path,
                          VkShaderModule* out_shader_module);

    class PipelineBuilder
    {
      public:
        PipelineBuilder();

        void Clear();

        PipelineBuilder SetName(const char* name);
        PipelineBuilder SetLayout(VkPipelineLayout layout);
        PipelineBuilder AddVertexShader(VkShaderModule shader);
        PipelineBuilder AddFragmentShader(VkShaderModule shader);
        PipelineBuilder SetInputTopology(VkPrimitiveTopology topology);
        PipelineBuilder SetPolygonMode(VkPolygonMode mode);
        PipelineBuilder SetCullMode(VkCullModeFlags cull_mode, VkFrontFace front_face);
        PipelineBuilder SetMultisamplingNone();
        PipelineBuilder DisableBlending();
        PipelineBuilder EnableBlendingAdditive();
        PipelineBuilder EnableBlendingAlpha();
        PipelineBuilder SetColorAttachmentFormat(VkFormat format);
        PipelineBuilder SetDepthFormat(VkFormat format);
        PipelineBuilder DisableDepthTest();
        PipelineBuilder EnableDepthTest(VkCompareOp compare_op = VK_COMPARE_OP_LESS);
        VkPipeline BuildPipeline(const vkb::DispatchTable& device_dispatch);

      private:
        std::vector<VkPipelineShaderStageCreateInfo> m_stages{};

        VkPipelineInputAssemblyStateCreateInfo m_input_assembly;
        VkPipelineRasterizationStateCreateInfo m_rasteriser;
        VkPipelineColorBlendAttachmentState m_color_blend_attachment;
        VkPipelineMultisampleStateCreateInfo m_multi_sampling;
        VkPipelineDepthStencilStateCreateInfo m_depth_stencil;
        VkPipelineRenderingCreateInfo m_render_info;
        VkPipelineLayout m_pipeline_layout;
        VkFormat m_color_attachment_format;

        const char* m_name;
    };
} // namespace Renderer::Utils