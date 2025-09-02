#pragma once

#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

namespace Renderer::Debug
{
    void DrawSceneContentsImGui(VulkanEngine& engine, Scene& scene);

    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<AllocatedImage>& image_storage);
    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<AllocatedBuffer>& buffer_storage);
    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<MeshAsset>& mesh_storage);
} // namespace Renderer::Debug