#pragma once

#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

namespace Renderer::Debug
{
    void DrawSceneContentsImGui(Scene& scene);

    void DrawStorageTableImGui(ResourceStorage<AllocatedImage>& image_storage);
    void DrawStorageTableImGui(ResourceStorage<AllocatedBuffer>& buffer_storage);
    void DrawStorageTableImGui(ResourceStorage<MeshAsset>& mesh_storage);
} // namespace Renderer::Debug