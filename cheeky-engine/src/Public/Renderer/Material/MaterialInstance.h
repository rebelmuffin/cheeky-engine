#pragma once

#include "Renderer/Material/MaterialPass.h"
#include "Renderer/VkTypes.h"

#include <vector>

namespace Renderer
{
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
} // namespace Renderer