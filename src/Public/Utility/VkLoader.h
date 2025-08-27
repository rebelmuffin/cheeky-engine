#pragma once

#include "VkTypes.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct GeoSurface
{
    uint32_t first_index;
    uint32_t index_count;
};

struct MeshAsset
{
    std::string name;

    GPUMeshBuffers buffers;
    std::vector<GeoSurface> surfaces;

    // #TODO: RAII destroy buffers
};

class VulkanEngine;

namespace Utils
{
    /// Loads meshes from a glTF file. Supports both binary and json gltf. Returns nullopt on failure.
    std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(VulkanEngine* engine,
                                                                          std::filesystem::path filePath);

} // namespace Utils