#pragma once

#include "Renderer/Material.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/fastgltf.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Renderer
{
    struct GLTFMaterial
    {
        MaterialInstance material;
    };

    // #TODO: rename to IndexedGeometry
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

    struct Viewport;
    class VulkanEngine;

    void DestroyMeshAsset(VulkanEngine& engine, const MeshAsset& asset);

    template <>
    inline void ResourceStorage<MeshAsset>::DestroyResource(VulkanEngine& engine, const MeshAsset& asset)
    {
        DestroyMeshAsset(engine, asset);
    }

    using MeshHandle = ReferenceCountedHandle<MeshAsset>;

    struct GLTFNode
    {
        std::vector<GLTFNode> children{};
        std::size_t scene_node_idx{};
        glm::mat4 transform{};
    };

    struct GLTFScene
    {
        std::vector<ImageHandle> loaded_textures{};
        std::vector<std::shared_ptr<GLTFMaterial>> loaded_materials{};
        std::vector<MeshHandle> loaded_meshes{};
        std::vector<fastgltf::Node> scene_nodes{};

        // hierarchical representation of the scene nodes. The root node itself is not a real node, iterate
        // through its children instead.
        std::optional<GLTFNode> root_node{};
    };
} // namespace Renderer

namespace Renderer::Utils
{
    std::optional<ImageHandle> LoadImageFromPath(
        VulkanEngine& engine, const char* path, const char* debug_name
    );

    /// Loads meshes from a glTF file. Supports both binary and json gltf. Returns nullopt on failure.
    std::optional<std::vector<MeshHandle>> LoadGltfMeshes(
        VulkanEngine* engine, std::filesystem::path file_path
    );

    std::optional<GLTFScene> LoadGltfScene(VulkanEngine& engine, std::filesystem::path file_path);
} // namespace Renderer::Utils