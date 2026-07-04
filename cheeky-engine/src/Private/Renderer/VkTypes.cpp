#include "Renderer/VkTypes.h"
#include "Renderer/VkEngine.h"

namespace Renderer
{
    void DestroyImage(VulkanEngine& engine, const AllocatedImage& image) { engine.DestroyImage(image); }

    void DestroyBuffer(VulkanEngine& engine, const AllocatedBuffer& buffer) { engine.DestroyBuffer(buffer); }
} // namespace Renderer