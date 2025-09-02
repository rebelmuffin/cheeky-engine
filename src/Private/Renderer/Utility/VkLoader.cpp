#include "Renderer/Utility/VkLoader.h"
#include "Renderer/Material.h"
#include "Renderer/Renderable.h"
#include "Renderer/Utility/VkInitialisers.h"
#include "Renderer/VkEngine.h"

#include "Renderer/VkTypes.h"
#include "ThirdParty/fastgltf.h"
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float4.hpp"
#include "vulkan/vulkan_core.h"
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>

#include <iostream>
#include <memory>
#include <utility>
#include <variant>

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

    std::optional<Renderer::ImageHandle> LoadGltfImage(
        Renderer::VulkanEngine& engine, const fastgltf::Asset& asset, const fastgltf::Image& image
    )
    {
        std::optional<Renderer::ImageHandle> out_handle = std::nullopt;
        std::visit(
            fastgltf::visitor{
                [&](const fastgltf::sources::URI& uri)
                {
                    if (uri.uri.isLocalPath())
                    {
                        out_handle =
                            Renderer::Utils::LoadImageFromPath(engine, uri.uri.c_str(), image.name.data());
                    }
                },
                [&](const fastgltf::sources::BufferView& view)
                {
                    const fastgltf::BufferView& buffer_view = asset.bufferViews[view.bufferViewIndex];
                    const fastgltf::Buffer& buffer = asset.buffers[buffer_view.bufferIndex];
                    // we expect this to have been loaded with LoadExternalBuffers which would have loaded the
                    // buffer into a std vector.
                    std::visit(
                        fastgltf::visitor{ [](const auto&)
                                           {
                                           },
                                           [&](const fastgltf::sources::Array& vec)
                                           {
                                               int width, height, channels;
                                               constexpr int desired_channels = 4;
                                               unsigned char* image_data = stbi_load_from_memory(
                                                   reinterpret_cast<const unsigned char*>(
                                                       vec.bytes.data() + buffer_view.byteOffset
                                                   ),
                                                   (int)vec.bytes.size(),
                                                   &width,
                                                   &height,
                                                   &channels,
                                                   desired_channels
                                               );
                                               if (image_data == nullptr)
                                               {
                                                   return;
                                               }

                                               VkExtent3D extents{ (uint32_t)width, (uint32_t)height, 1 };
                                               out_handle = engine.AllocateImage(
                                                   image_data,
                                                   extents,
                                                   VK_FORMAT_R8G8B8A8_UNORM,
                                                   VK_IMAGE_USAGE_SAMPLED_BIT,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                   true,
                                                   image.name.data()
                                               );

                                               stbi_image_free(image_data);
                                           } },
                        buffer.data
                    );
                },
                [](const auto&)
                {
                } },
            image.data
        );

        return out_handle;
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
    std::optional<ImageHandle> LoadImageFromPath(
        VulkanEngine& engine, const char* path, const char* debug_name
    )
    {
        int width, height, channels;
        unsigned char* image_data = stbi_load(path, &width, &height, &channels, 4);
        if (image_data == nullptr)
        {
            return std::nullopt;
        }

        VkExtent3D image_extents{ (uint32_t)width, (uint32_t)height, 1 };
        ImageHandle loaded_image = engine.AllocateImage(
            image_data,
            image_extents,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            true,
            debug_name
        );

        stbi_image_free(image_data);
        return loaded_image;
    }

    std::optional<std::vector<MeshHandle>> LoadGltfMeshes(
        VulkanEngine* engine, std::filesystem::path file_path
    )
    {
        std::optional<fastgltf::Asset> opt_asset = FastGltfLoadAsset(file_path);
        if (opt_asset == std::nullopt)
        {
            return std::nullopt;
        }

        fastgltf::Asset& asset = *opt_asset;
        std::vector<MeshHandle> mesh_assets;
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

            mesh_asset.buffers = engine->UploadMesh(indices, vertices);
            mesh_assets.emplace_back(engine->RegisterMeshAsset(std::move(mesh_asset), mesh.name));
        }

        return mesh_assets;
    }

    bool LoadGltfIntoScene(Scene& scene, VulkanEngine& engine, std::filesystem::path file_path)
    {
        // we want to load the textures in as well, so we can create the materials with correct textures
        const std::optional<const fastgltf::Asset>& opt_asset = FastGltfLoadAsset(
            file_path,
            fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalImages |
                fastgltf::Options::LoadExternalBuffers
        );
        if (opt_asset == std::nullopt)
        {
            return false;
        }
        const fastgltf::Asset& asset = *opt_asset;

        std::vector<ImageHandle> out_images{};
        std::vector<std::shared_ptr<GLTFMaterial>> out_materials{};
        std::vector<MeshHandle> out_meshes{};

        for (const fastgltf::Texture& gltf_texture : asset.textures)
        {
            const fastgltf::Image& image = asset.images[gltf_texture.imageIndex.value()];
            std::optional<ImageHandle> out_image = LoadGltfImage(engine, asset, image);

            // default to placeholder image
            out_images.emplace_back(out_image.value_or(engine.PlaceholderImage()));
        }

        // create a default material for surfaces that don't have one.
        Material_GLTF_PBR::MaterialParameters default_mat_params;
        default_mat_params.colour = glm::vec4(1.0f);
        default_mat_params.metal_roughness = glm::vec4(1.0f);
        BufferHandle default_mat_uniform = engine.CreateBuffer(
            &default_mat_params,
            sizeof(default_mat_params),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            "default material uniform buffer"
        );

        Material_GLTF_PBR::Resources default_mat_resources;
        default_mat_resources.colour_image = engine.PlaceholderImage();
        default_mat_resources.colour_sampler = engine.Sampler();
        default_mat_resources.metal_roughness_image = engine.PlaceholderImage();
        default_mat_resources.metal_roughness_sampler = engine.Sampler();
        default_mat_resources.uniform_buffer = default_mat_uniform;
        default_mat_resources.buffer_offset = 0;

        std::shared_ptr<GLTFMaterial> default_material = std::make_shared<GLTFMaterial>();
        default_material->material = engine.PBRMaterial().CreateInstance(
            engine.DeviceDispatchTable(),
            MaterialPass::MainColour,
            default_mat_resources,
            engine.PBRMaterial().descriptor_allocator
        );

        for (const fastgltf::Material& gltf_mat : asset.materials)
        {
            fastgltf::math::nvec4 colour_factor = gltf_mat.pbrData.baseColorFactor;
            float metal_roughness_factor = gltf_mat.pbrData.roughnessFactor;

            Material_GLTF_PBR::MaterialParameters mat_params;
            mat_params.colour =
                glm::vec4(colour_factor.x(), colour_factor.y(), colour_factor.z(), colour_factor.w());
            mat_params.metal_roughness = glm::vec4(metal_roughness_factor); // #TODO: figure this out later
            BufferHandle mat_uniform = engine.CreateBuffer(
                &mat_params, sizeof(mat_params), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, gltf_mat.name.data()
            );

            ImageHandle colour_image = engine.WhiteImage();
            ImageHandle metal_roughness_image = engine.WhiteImage();

            if (gltf_mat.pbrData.baseColorTexture.has_value())
            {
                size_t idx = gltf_mat.pbrData.baseColorTexture->textureIndex;
                colour_image = out_images[idx];
            }
            if (gltf_mat.pbrData.metallicRoughnessTexture.has_value())
            {
                size_t idx = gltf_mat.pbrData.metallicRoughnessTexture->textureIndex;
                metal_roughness_image = out_images[idx];
            }

            Material_GLTF_PBR::Resources mat_resources;
            mat_resources.colour_image = colour_image;
            mat_resources.colour_sampler = engine.Sampler();
            mat_resources.metal_roughness_image = metal_roughness_image;
            mat_resources.metal_roughness_sampler = engine.Sampler();
            mat_resources.uniform_buffer = mat_uniform;
            mat_resources.buffer_offset = 0;

            MaterialPass pass;
            switch (gltf_mat.alphaMode)
            {
            case fastgltf::AlphaMode::Blend:
                pass = MaterialPass::Transparent;
                break;
            case fastgltf::AlphaMode::Opaque:
                pass = MaterialPass::MainColour;
                break;
            case fastgltf::AlphaMode::Mask:
                pass = MaterialPass::Other;
                break;
            }

            std::shared_ptr<GLTFMaterial> new_mat =
                out_materials.emplace_back(std::make_shared<GLTFMaterial>());
            new_mat->material = engine.PBRMaterial().CreateInstance(
                engine.DeviceDispatchTable(), pass, mat_resources, engine.PBRMaterial().descriptor_allocator
            );
        }

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

                if (primitive.materialIndex.has_value())
                {
                    surface.material = out_materials[primitive.materialIndex.value()];
                }
                else
                {
                    surface.material = default_material;
                }

                mesh_asset.surfaces.emplace_back(surface);
            }

            mesh_asset.buffers = engine.UploadMesh(indices, vertices);
            MeshHandle created_mesh = engine.RegisterMeshAsset(std::move(mesh_asset), mesh.name);
            out_meshes.emplace_back(created_mesh);
        }

        // now just create scene items from these
        for (const fastgltf::Node& node : asset.nodes)
        {
            if (node.meshIndex.has_value() == false)
            {
                continue;
            }

            // ignore the hierarchy for now and see what happens
            MeshSceneItem item{};
            item.name = node.name;
            item.asset = out_meshes[*node.meshIndex];
            // our matrices should be compatible with gltf
            fastgltf::TRS trs = std::get<fastgltf::TRS>(node.transform);
            glm::mat4 transform = glm::translate(
                glm::mat4(1.0f), glm::vec3(trs.translation.x(), trs.translation.y(), trs.translation.z())
            );
            transform *=
                glm::mat4(glm::quat(trs.rotation.x(), trs.rotation.y(), trs.rotation.z(), trs.rotation.w()));
            transform = glm::scale(transform, glm::vec3(trs.scale.x(), trs.scale.y(), trs.scale.z()));
            item.transform = transform;

            scene.scene_items.emplace_back(std::make_unique<MeshSceneItem>(std::move(item)));
        }

        if (asset.nodes.empty())
        {
            // if empty, just spit out all the meshes as individual nodes.
            for (const MeshHandle& mesh : out_meshes)
            {
                MeshSceneItem item{};
                item.name = mesh->name;
                item.asset = mesh;
                item.transform = glm::mat4(1.0f);
                scene.scene_items.emplace_back(std::make_unique<MeshSceneItem>(std::move(item)));
            }
        }

        return true;
    }
} // namespace Renderer::Utils