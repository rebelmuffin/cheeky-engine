#pragma once

#include "Renderer/Material.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/VkTypes.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Renderer
{
    struct GLTFMaterial
    {
        MaterialInstance material;
    };

    struct GeoSurface
    {
        uint32_t first_index;
        uint32_t index_count;
        std::shared_ptr<GLTFMaterial> material;
    };

    struct MeshAsset
    {
        std::string name;

        GPUMeshBuffers buffers;
        std::vector<GeoSurface> surfaces;
    };

    struct Scene;
    class VulkanEngine;

    void DestroyMeshAsset(VulkanEngine& engine, const MeshAsset& asset);

    template <>
    inline void ResourceStorage<MeshAsset>::DestroyResource(VulkanEngine& engine, const MeshAsset& asset)
    {
        DestroyMeshAsset(engine, asset);
    }

} // namespace Renderer

namespace Renderer::Utils
{
    /// Loads meshes from a glTF file. Supports both binary and json gltf. Returns nullopt on failure.
    std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(
        VulkanEngine* engine, std::filesystem::path file_path
    );

    bool LoadGltfIntoScene(Scene& scene, VulkanEngine& engine, std::filesystem::path file_path);

} // namespace Renderer::Utils