#pragma once

#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

namespace Renderer::Debug
{
    void DrawViewportContentsImGui(VulkanEngine& engine, Viewport& viewport);

    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<AllocatedImage>& image_storage);
    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<AllocatedBuffer>& buffer_storage);
    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<MeshAsset>& mesh_storage);
} // namespace Renderer::Debug