#include "Renderer/Utility/VkLoader.h"
#include "Renderer/Material.h"
#include "Renderer/Utility/VkInitialisers.h"
#include "Renderer/VkEngine.h"

#include "Renderer/VkTypes.h"
#include "ThirdParty/fastgltf.h"
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include <glm/gtx/quaternion.hpp>
#include <memory>

#include <iostream>

namespace
{
    void PrintFastGltfError(const char* message, fastgltf::Error error)
    {
        std::cout << message << '\t' << fastgltf::getErrorName(error) << ": "
                  << fastgltf::getErrorMessage(error) << std::endl;
    }

    std::optional<fastgltf::Asset> FastGltfLoadAsset(
        std::filesystem::path file_path, fastgltf::Options extra_options = fastgltf::Options::None
    )
    {
        std::cout << "[*] Loading glTF file: " << file_path << std::endl;

        fastgltf::Expected<fastgltf::GltfDataBuffer> load_result =
            fastgltf::GltfDataBuffer::FromPath(file_path);
        if (load_result.error() != fastgltf::Error::None)
        {
            PrintFastGltfError("[!] Failed to load glTF file: ", load_result.error());
            return std::nullopt;
        }

        const fastgltf::Options loading_options = extra_options | fastgltf::Options::LoadExternalBuffers;

        fastgltf::Parser parser;

        fastgltf::Expected<fastgltf::Asset> parse_result = parser.loadGltf(
            load_result.get(), file_path.parent_path(), loading_options, fastgltf::Category::Meshes
        );
        if (parse_result.error() != fastgltf::Error::None)
        {
            PrintFastGltfError("[!] Failed to parse glTF file: ", parse_result.error());
            return std::nullopt;
        }

        return std::move(parse_result.get());
    }

    bool LoadPrimitiveIndicesVertices(
        const fastgltf::Asset& asset,
        const fastgltf::Primitive& primitive,
        std::vector<uint32_t>& indices,
        std::vector<Renderer::Vertex>& vertices,
        Renderer::GeoSurface& surface,
        const char*& out_error_mesage
    )
    {
        surface.first_index = static_cast<uint32_t>(indices.size());
        surface.index_count = static_cast<uint32_t>(asset.accessors[primitive.indicesAccessor.value()].count);

        const size_t initial_vertex = vertices.size();

        // load indices
        {
            const fastgltf::Accessor& index_accessor = asset.accessors[primitive.indicesAccessor.value()];
            indices.reserve(indices.size() + index_accessor.count);

            fastgltf::iterateAccessor<uint32_t>(
                asset,
                index_accessor,
                [&](uint32_t idx)
                {
                    indices.emplace_back(idx);
                }
            );
        }

        // accessor indices
        const fastgltf::Attribute* position_attribute = primitive.findAttribute("POSITION");
        const fastgltf::Attribute* normal_attribute = primitive.findAttribute("NORMAL");
        const fastgltf::Attribute* uv_attribute = primitive.findAttribute("TEXCOORD_0");
        const fastgltf::Attribute* colour_attribute = primitive.findAttribute("COLOR_0");

        if (position_attribute == primitive.attributes.cend())
        {
            out_error_mesage = "[!] Mesh primitive does not have a POSITION attribute";
            return false;
        }

        // load vertex positions & create them for the first time
        {
            const fastgltf::Accessor& position_accessor = asset.accessors[position_attribute->accessorIndex];
            vertices.resize(vertices.size() + position_accessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset,
                position_accessor,
                [&](fastgltf::math::fvec3 pos, std::size_t idx)
                {
                    Renderer::Vertex new_vertex{};
                    new_vertex.position = { pos.x(), pos.y(), pos.z() };

                    // everything else will be populated during later iterations
                    new_vertex.normal = { 0.0f, 0.0f, 0.0f };
                    new_vertex.colour = glm::vec4(1.0f);
                    new_vertex.uv_x = 0.0f;
                    new_vertex.uv_y = 0.0f;
                    vertices[initial_vertex + idx] = new_vertex;
                }
            );
        }

        // load vertex normals into existing vertices
        if (normal_attribute != primitive.attributes.cend())
        {
            const fastgltf::Accessor& normal_accessor = asset.accessors[normal_attribute->accessorIndex];

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset,
                normal_accessor,
                [&](fastgltf::math::fvec3 normal, std::size_t idx)
                {
                    vertices[initial_vertex + idx].normal = { normal.x(), normal.y(), normal.z() };
                }
            );
        }

        // load vertex uvs into existing vertices
        if (uv_attribute != primitive.attributes.cend())
        {
            const fastgltf::Accessor& uv_accessor = asset.accessors[uv_attribute->accessorIndex];
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                asset,
                uv_accessor,
                [&](fastgltf::math::fvec2 uv, std::size_t idx)
                {
                    vertices[initial_vertex + idx].uv_x = uv.x();
                    vertices[initial_vertex + idx].uv_y = uv.y();
                }
            );
        }

        // load vertex colours into existing vertices
        if (colour_attribute != primitive.attributes.cend())
        {
            const fastgltf::Accessor& colour_accessor = asset.accessors[colour_attribute->accessorIndex];
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                asset,
                colour_accessor,
                [&](fastgltf::math::fvec4 col, std::size_t idx)
                {
                    vertices[initial_vertex + idx].colour = { col.x(), col.y(), col.z(), col.w() };
                }
            );
        }

        return true;
    }
} // namespace

namespace Renderer
{
    void DestroyMeshAsset(VulkanEngine&, const MeshAsset&)
    {
        // there's actually nothing to delete for a mesh asset at the moment. The only things it contains that
        // needs deletion are the buffers and those are already reference counted.
    }
} // namespace Renderer

namespace Renderer::Utils
{
    std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(
        VulkanEngine* engine, std::filesystem::path file_path
    )
    {
        std::optional<fastgltf::Asset> opt_asset = FastGltfLoadAsset(file_path);
        if (opt_asset == std::nullopt)
        {
            return std::nullopt;
        }

        fastgltf::Asset& asset = *opt_asset;
        std::vector<std::shared_ptr<MeshAsset>> mesh_assets;
        mesh_assets.reserve(asset.meshes.size());

        // we re-use the same vectors to avoid re-allocating them for each mesh
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;
        for (const fastgltf::Mesh& mesh : asset.meshes)
        {
            indices.clear();
            vertices.clear();

            MeshAsset mesh_asset;
            mesh_asset.name = mesh.name;

            for (const fastgltf::Primitive& primitive : mesh.primitives)
            {
                GeoSurface surface;
                // any errors will be written here.
                const char* error_message = "";
                bool result =
                    LoadPrimitiveIndicesVertices(asset, primitive, indices, vertices, surface, error_message);
                if (result == false)
                {
                    std::cout << error_message << "\nSkipping mesh: " << mesh_asset.name << std::endl;
                    continue;
                }

                mesh_asset.surfaces.emplace_back(surface);
            }

            constexpr bool visualise_normals = false;
            if constexpr (visualise_normals)
            {
                // for debugging: visualise normals as colours
                for (Vertex& v : vertices)
                {
                    v.colour = glm::vec4(v.normal, 1.0f);
                }
            }

            mesh_asset.buffers = engine->UploadMesh(indices, vertices);
            mesh_assets.emplace_back(std::make_shared<MeshAsset>(std::move(mesh_asset)));
        }

        return mesh_assets;
    }

    bool LoadGltfIntoScene(Scene& /*scene*/, VulkanEngine& /*engine*/, std::filesystem::path file_path)
    {
        // we want to load the textures in as well, so we can create the materials with correct textures
        const std::optional<const fastgltf::Asset>& opt_asset =
            FastGltfLoadAsset(file_path, fastgltf::Options::LoadExternalImages);
        if (opt_asset == std::nullopt)
        {
            return false;
        }

        std::vector<std::shared_ptr<GLTFMaterial>> out_material{};

        // create a default material for surfaces that don't have one.
        std::shared_ptr<GLTFMaterial> default_material;

        // const fastgltf::Asset& asset = *opt_asset;
        // for (const fastgltf::Material& material : asset.materials)
        // {
        // }

        return true;
    }
} // namespace Renderer::Utils