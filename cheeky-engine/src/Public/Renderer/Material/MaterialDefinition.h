#pragma once

#include "MaterialPass.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Renderer
{
    enum class MaterialPipelinePolygonMode
    {
        Line,
        Fill
    };

    enum class MaterialPipelineTopology
    {
        Line,
        Triangle
    };

    struct MaterialPipelineDefinition
    {
        bool enable_transparency = false;
        bool enable_depth = true;
        MaterialPipelinePolygonMode polygon_mode = MaterialPipelinePolygonMode::Fill;
        MaterialPipelineTopology topology = MaterialPipelineTopology::Triangle;
    };

    enum class MaterialBindingType
    {
        ImageSampler,
        UniformBuffer
    };

    struct MaterialDescriptorBinding
    {
        MaterialBindingType type{};
        size_t size{}; // for uniform buffers
    };

    struct MaterialDescriptorDefinition
    {
        std::vector<MaterialDescriptorBinding> bindings{};
    };

    // Structure that contains enough information to construct a material and its resources.
    // This is an abstraction of Vulkan structures to make custom materials easier to create.
    struct MaterialDefinition
    {
        std::unordered_map<MaterialPass, MaterialPipelineDefinition> pipelines{};
        MaterialDescriptorDefinition descriptor{};
        std::string frag_shader{};
        std::string vert_shader{};
    };
} // namespace Renderer